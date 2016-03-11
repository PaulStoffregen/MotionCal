#include "imuread.h"


#define HIST_LEN 12
uint32_t mag_histogram[3][65536];
int raw_history[9][HIST_LEN];
int raw_history_count=0;
int raw_history_last=0;

void raw_data_reset(void)
{


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

	//printf("raw_data: %d %d %d %d %d %d %d %d %d\n", data[0], data[1], data[2],
		//data[3], data[4], data[5], data[6], data[7], data[8]);
	mag_histogram[0][data[6]+32786]++;
	mag_histogram[1][data[7]+32786]++;
	mag_histogram[2][data[8]+32786]++;

	// RdSensData_Run (tasks.c) - reads 8 samples, sums, scales to sci units
	// Fusion_Run (tasks.c)
	//   fInvertMagCal - applies hard & soft iron cal
	//     fV[3]       = hard iron -- ftrV[3]
        //     finvW[3][3] = soft iron
	//   iUpdateMagnetometerBuffer - adds data to buffer
	//   fRun_6DOF_GY_KALMAN
	//   fRun_9DOF_GBY_KALMAN
	// MagCal_Run (tasks.c)
	//   fUpdateCalibration4INV
	//   fUpdateCalibration7EIG
	//   fUpdateCalibration10EIG

	add_magcal_data(data);
	MagCal_Run();
}




