#include "imuread.h"


static int rawcount=OVERSAMPLE_RATIO;
static AccelSensor_t accel;
static MagSensor_t   mag;
static GyroSensor_t  gyro;

void raw_data_reset(void)
{
	rawcount = OVERSAMPLE_RATIO;
	fusion_init();
	memset(&magcal, 0, sizeof(magcal));
	magcal.V[2] = 80.0f;  // initial guess
	magcal.invW[0][0] = 1.0f;
	magcal.invW[1][1] = 1.0f;
	magcal.invW[2][2] = 1.0f;
	magcal.FitError = 100.0f;
	magcal.FitErrorAge = 100.0f;
	magcal.B = 50.0f;
}

static int choose_discard_magcal(void)
{
	int32_t rawx, rawy, rawz;
	int32_t dx, dy, dz;
	float x, y, z;
	uint64_t distsq, minsum=0xFFFFFFFFFFFFFFFF;
	static int runcount=0;
	int i, j, minindex=0;
	Point_t point;
	float gaps, field, error, errormax;

	// When enough data is collected (gaps error is low), assume we
	// have a pretty good coverage and the field stregth is known.
	gaps = quality_surface_gap_error();
	if (gaps < 25.0f) {
		// occasionally look for points farthest from average field strength
		// always rate limit assumption-based data purging, but allow the
		// rate to increase as the angular coverage improves.
		if (gaps < 1.0f) gaps = 1.0f;
		if (++runcount > (int)(gaps * 10.0f)) {
			j = MAGBUFFSIZE;
			errormax = 0.0f;
			for (i=0; i < MAGBUFFSIZE; i++) {
				rawx = magcal.BpFast[0][i];
				rawy = magcal.BpFast[1][i];
				rawz = magcal.BpFast[2][i];
				apply_calibration(rawx, rawy, rawz, &point);
				x = point.x;
				y = point.y;
				z = point.z;
				field = sqrtf(x * x + y * y + z * z);
				// if magcal.B is bad, things could go horribly wrong
				error = fabsf(field - magcal.B);
				if (error > errormax) {
					errormax = error;
					j = i;
				}
			}
			runcount = 0;
			if (j < MAGBUFFSIZE) {
				//printf("worst error at %d\n", j);
				return j;
			}
		}
	} else {
		runcount = 0;
	}
	// When solid info isn't availabe, find 2 points closest to each other,
	// and randomly discard one.  When we don't have good coverage, this
	// approach tends to add points into previously unmeasured areas while
	// discarding info from areas with highly redundant info.
	for (i=0; i < MAGBUFFSIZE; i++) {
		for (j=i+1; j < MAGBUFFSIZE; j++) {
			dx = magcal.BpFast[0][i] - magcal.BpFast[0][j];
			dy = magcal.BpFast[1][i] - magcal.BpFast[1][j];
			dz = magcal.BpFast[2][i] - magcal.BpFast[2][j];
			distsq = (int64_t)dx * (int64_t)dx;
			distsq += (int64_t)dy * (int64_t)dy;
			distsq += (int64_t)dz * (int64_t)dz;
			if (distsq < minsum) {
				minsum = distsq;
				minindex = (random() & 1) ? i : j;
			}
		}
	}
	return minindex;
}


static void add_magcal_data(const int16_t *data)
{
	int i;

	// first look for an unused caldata slot
	for (i=0; i < MAGBUFFSIZE; i++) {
		if (!magcal.valid[i]) break;
	}
	// If the buffer is full, we must choose which old data to discard.
	// We must choose wisely!  Throwing away the wrong data could prevent
	// collecting enough data distributed across the entire 3D angular
	// range, preventing a decent cal from ever happening at all.  Making
	// any assumption about good vs bad data is particularly risky,
	// because being wrong could cause an unstable feedback loop where
	// bad data leads to wrong decisions which leads to even worse data.
	// But if done well, purging bad data has massive potential to
	// improve results.  The trick is telling the good from the bad while
	// still in the process of learning what's good...
	if (i >= MAGBUFFSIZE) {
		i = choose_discard_magcal();
		if (i < 0 || i >= MAGBUFFSIZE) {
			i = random() % MAGBUFFSIZE;
		}
	}
	// add it to the cal buffer
	magcal.BpFast[0][i] = data[6];
	magcal.BpFast[1][i] = data[7];
	magcal.BpFast[2][i] = data[8];
	magcal.valid[i] = 1;
}

void cal1_data(const float *data)
{
#if 0
	int i;

	printf("got cal1_data:\n");
	for (i=0; i<10; i++) {
		printf("  %.5f\n", data[i]);
	}
#endif
}

void cal2_data(const float *data)
{
#if 0
	int i;

	printf("got cal2_data:\n");
	for (i=0; i<9; i++) {
		printf("  %.5f\n", data[i]);
	}
#endif
}

void raw_data(const int16_t *data)
{
	static int force_orientation_counter=0;
	float x, y, z, ratio, magdiff;
	Point_t point;

	add_magcal_data(data);
	x = magcal.V[0];
	y = magcal.V[1];
	z = magcal.V[2];
	if (MagCal_Run()) {
		x -= magcal.V[0];
		y -= magcal.V[1];
		z -= magcal.V[2];
		magdiff = sqrtf(x * x + y * y + z * z);
		//printf("magdiff = %.2f\n", magdiff);
		if (magdiff > 0.8f) {
			fusion_init();
			rawcount = OVERSAMPLE_RATIO;
			force_orientation_counter = 240;
		}
	}

	if (force_orientation_counter > 0) {
		if (--force_orientation_counter == 0) {
			//printf("delayed forcible orientation reset\n");
			fusion_init();
			rawcount = OVERSAMPLE_RATIO;
		}
	}

	if (rawcount >= OVERSAMPLE_RATIO) {
		memset(&accel, 0, sizeof(accel));
		memset(&mag, 0, sizeof(mag));
		memset(&gyro, 0, sizeof(gyro));
		rawcount = 0;
	}
	x = (float)data[0] * G_PER_COUNT;
	y = (float)data[1] * G_PER_COUNT;
	z = (float)data[2] * G_PER_COUNT;
	accel.GpFast[0] = x;
	accel.GpFast[1] = y;
	accel.GpFast[2] = y;
	accel.Gp[0] += x;
	accel.Gp[1] += y;
	accel.Gp[2] += y;

	x = (float)data[3] * DEG_PER_SEC_PER_COUNT;
	y = (float)data[4] * DEG_PER_SEC_PER_COUNT;
	z = (float)data[5] * DEG_PER_SEC_PER_COUNT;
	gyro.Yp[0] += x;
	gyro.Yp[1] += y;
	gyro.Yp[2] += z;
	gyro.YpFast[rawcount][0] = x;
	gyro.YpFast[rawcount][1] = y;
	gyro.YpFast[rawcount][2] = z;

	apply_calibration(data[6], data[7], data[8], &point);
	mag.BcFast[0] = point.x;
	mag.BcFast[1] = point.y;
	mag.BcFast[2] = point.z;
	mag.Bc[0] += point.x;
	mag.Bc[1] += point.y;
	mag.Bc[2] += point.z;

	rawcount++;
	if (rawcount >= OVERSAMPLE_RATIO) {
		ratio = 1.0f / (float)OVERSAMPLE_RATIO;
		accel.Gp[0] *= ratio;
		accel.Gp[1] *= ratio;
		accel.Gp[2] *= ratio;
		gyro.Yp[0] *= ratio;
		gyro.Yp[1] *= ratio;
		gyro.Yp[2] *= ratio;
		mag.Bc[0] *= ratio;
		mag.Bc[1] *= ratio;
		mag.Bc[2] *= ratio;
		fusion_update(&accel, &mag, &gyro, &magcal);
		fusion_read(&current_orientation);
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
	uint8_t *p, buf[68];
	uint16_t crc;
	int i;

	p = buf;
	*p++ = 117; // 2 byte signature
	*p++ = 84;
	for (i=0; i < 3; i++) {
		p = copy_lsb_first(p, 0.0f); // accelerometer offsets
	}
	for (i=0; i < 3; i++) {
		p = copy_lsb_first(p, 0.0f); // gyroscope offsets
	}
	for (i=0; i < 3; i++) {
		p = copy_lsb_first(p, magcal.V[i]); // 12 bytes offset/hardiron
	}
	p = copy_lsb_first(p, magcal.B); // field strength
	p = copy_lsb_first(p, magcal.invW[0][0]); //10
	p = copy_lsb_first(p, magcal.invW[1][1]); //11
	p = copy_lsb_first(p, magcal.invW[2][2]); //12
	p = copy_lsb_first(p, magcal.invW[0][1]); //13
	p = copy_lsb_first(p, magcal.invW[0][2]); //14
	p = copy_lsb_first(p, magcal.invW[1][2]); //15
	crc = 0xFFFF;
	for (i=0; i < 66; i++) {
		crc = crc16(crc, buf[i]);
	}
	*p++ = crc;   // 2 byte crc check
	*p++ = crc >> 8;
	return write_serial_data(buf, 68);
}

