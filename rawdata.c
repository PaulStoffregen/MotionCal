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
	magcal.fV[1] = 10.0f;
	magcal.fV[2] = 80.0f;  // initial guess
	magcal.finvW[0][0] = 1.0f;
	magcal.finvW[1][1] = 1.0f;
	magcal.finvW[2][2] = 1.0f;
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



