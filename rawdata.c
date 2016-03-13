#include "imuread.h"


static int rawcount=OVERSAMPLE_RATIO;
static AccelSensor_t accel;
static MagSensor_t   mag;
static GyroSensor_t  gyro;
SV_9DOF_GBY_KALMAN_t fusionstate;

void raw_data_reset(void)
{
	rawcount = OVERSAMPLE_RATIO;
	fInit_9DOF_GBY_KALMAN(&fusionstate, 100, OVERSAMPLE_RATIO);
	memset(&magcal, 0, sizeof(magcal));
	magcal.fV[2] = 80.0f;  // initial guess
	magcal.finvW[0][0] = 1.0f;
	magcal.finvW[1][1] = 1.0f;
	magcal.finvW[2][2] = 1.0f;
	magcal.fFitErrorpc = 100.0f;
	magcal.fFitErrorAge = 100.0f;
}

static void add_magcal_data(const int16_t *data)
{
	int32_t dx, dy, dz;
	uint64_t distsq, minsum=0xFFFFFFFFFFFFFFFF;
	int i, j, minindex=0;

	// first look for an unused caldata slot
	for (i=0; i < MAGBUFFSIZE; i++) {
		if (!magcal.valid[i]) break;
	}
	// if no unused, find the ones closest to each other
	// TODO: after reasonable sphere fit, we should retire older data
	// and choose the ones farthest from the sphere's radius
	if (i >= MAGBUFFSIZE) {
		for (i=0; i < MAGBUFFSIZE; i++) {
			for (j=i+1; j < MAGBUFFSIZE; j++) {
				dx = magcal.iBpFast[0][i] - magcal.iBpFast[0][j];
				dy = magcal.iBpFast[1][i] - magcal.iBpFast[1][j];
				dz = magcal.iBpFast[2][i] - magcal.iBpFast[2][j];
				distsq = (int64_t)dx * (int64_t)dx;
				distsq += (int64_t)dy * (int64_t)dy;
				distsq += (int64_t)dz * (int64_t)dz;
				if (distsq < minsum) {
					minsum = distsq;
					minindex = (random() & 1) ? i : j;
				}
			}
		}
		i = minindex;
	}
	// add it to the cal buffer
	magcal.iBpFast[0][i] = data[6];
	magcal.iBpFast[1][i] = data[7];
	magcal.iBpFast[2][i] = data[8];
	magcal.valid[i] = 1;
}


void raw_data(const int16_t *data)
{
	float x, y, z, ratio;
	Point_t point;

	add_magcal_data(data);
	MagCal_Run();

	if (rawcount >= OVERSAMPLE_RATIO) {
		memset(&accel, 0, sizeof(accel));
		memset(&mag, 0, sizeof(mag));
		memset(&gyro, 0, sizeof(gyro));
		rawcount = 0;
	}
	x = (float)data[0] * G_PER_COUNT;
	y = (float)data[1] * G_PER_COUNT;
	z = (float)data[2] * G_PER_COUNT;
	accel.fGpFast[0] = x;
	accel.fGpFast[1] = y;
	accel.fGpFast[2] = y;
	accel.fGp[0] += x;
	accel.fGp[1] += y;
	accel.fGp[2] += y;

	x = (float)data[3] * DEG_PER_SEC_PER_COUNT;
	y = (float)data[4] * DEG_PER_SEC_PER_COUNT;
	z = (float)data[5] * DEG_PER_SEC_PER_COUNT;
	gyro.fYp[0] += x;
	gyro.fYp[1] += y;
	gyro.fYp[2] += z;
	gyro.iYpFast[rawcount][0] = data[3];
	gyro.iYpFast[rawcount][1] = data[4];
	gyro.iYpFast[rawcount][2] = data[5];

	apply_calibration(data[6], data[7], data[8], &point);
	mag.fBcFast[0] = point.x;
	mag.fBcFast[1] = point.y;
	mag.fBcFast[2] = point.z;
	mag.fBc[0] += point.x;
	mag.fBc[1] += point.y;
	mag.fBc[2] += point.z;

	rawcount++;
	if (rawcount >= OVERSAMPLE_RATIO) {
		ratio = 1.0f / (float)OVERSAMPLE_RATIO;
		accel.fGp[0] *= ratio;
		accel.fGp[1] *= ratio;
		accel.fGp[2] *= ratio;
		gyro.fYp[0] *= ratio;
		gyro.fYp[1] *= ratio;
		gyro.fYp[2] *= ratio;
		mag.fBc[0] *= ratio;
		mag.fBc[1] *= ratio;
		mag.fBc[2] *= ratio;
		fRun_9DOF_GBY_KALMAN(&fusionstate, &accel, &mag, &gyro,
			&magcal, OVERSAMPLE_RATIO);

		memcpy(&current_orientation, &(fusionstate.fqPl), sizeof(Quaternion_t));
	}
}

static uint16_t crc16(uint16_t crc, uint8_t data)
{
        unsigned int i;

        crc ^= data;
        for (i = 0; i < 8; ++i) {
                if (crc & 1) {
                        crc = (crc >> 1) ^ 0xA001;
                } else {
                        crc = (crc >> 1);
                }
        }
        return crc;
}

static uint8_t * copy_lsb_first(uint8_t *dst, float f)
{
	union {
		float f;
		uint32_t n;
	} data;

	data.f = f;
	*dst++ = data.n;
	*dst++ = data.n >> 8;
	*dst++ = data.n >> 16;
	*dst++ = data.n >> 24;
	return dst;
}

int send_calibration(void)
{
	uint8_t *p, buf[52];
	uint16_t crc;
	int i, j;

	p = buf;
	*p++ = 117; // 2 byte signature
	*p++ = 84;
	for (i=0; i < 3; i++) {
		p = copy_lsb_first(p, magcal.fV[i]); // 12 bytes offset/hardiron
	}
	for (i=0; i < 3; i++) {
		for (j=0; j < 3; j++) {
			p = copy_lsb_first(p, magcal.finvW[i][j]); // 36 bytes softiron
		}
	}
	crc = 0xFFFF;
	for (i=0; i < 50; i++) {
		crc = crc16(crc, buf[i]);
	}
	*p++ = crc;   // 2 byte crc check
	*p++ = crc >> 8;
	return write_serial_data(buf, 52);
}
