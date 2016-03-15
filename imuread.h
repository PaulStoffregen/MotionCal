#ifndef IMUread_h_
#define IMUread_h_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if defined(LINUX)
  #include <termios.h>
  #include <unistd.h>
  #include <GL/gl.h>
  #include <GL/glu.h>
#elif defined(WINDOWS)
  #include <windows.h>
  #include <GL/gl.h>
  #include <GL/glu.h>
#elif defined(MACOSX)
  #include <termios.h>
  #include <unistd.h>
  #include <OpenGL/gl.h>
  #include <OpenGL/glu.h>
#endif


#if defined(LINUX)
  #define PORT "/dev/ttyACM0"
#elif defined(WINDOWS)
  #define PORT "COM3"
#elif defined(MACOSX)
  #define PORT "/dev/cu.usbmodemfd132"
#endif

#define TIMEOUT_MSEC 33

#define MAGBUFFSIZE 650 // Freescale's lib needs at least 392

#ifdef __cplusplus
extern "C"{
#endif

typedef struct {
	float x;
	float y;
	float z;
	//int valid;
} Point_t;

typedef struct {
	float q0; // w
	float q1; // x
	float q2; // y
	float q3; // z
} Quaternion_t;
extern Quaternion_t current_orientation;

extern int port_is_open(void);
extern int open_port(const char *name);
extern int read_serial_data(void);
extern int write_serial_data(const void *ptr, int len);
extern void close_port(void);
void raw_data_reset(void);
void raw_data(const int16_t *data);
int send_calibration(void);
void visualize_init(void);
void apply_calibration(int16_t rawx, int16_t rawy, int16_t rawz, Point_t *out);
void display_callback(void);
void resize_callback(int width, int height);
void MagCal_Run(void);


// magnetic calibration & buffer structure
typedef struct {
    float V[3];                  // current hard iron offset x, y, z, (uT)
    float invW[3][3];            // current inverse soft iron matrix
    float B;                     // current geomagnetic field magnitude (uT)
    float FourBsq;               // current 4*B*B (uT^2)
    float FitError;              // current fit error %
    float FitErrorAge;           // current fit error % (grows automatically with age)
    float trV[3];                // trial value of hard iron offset z, y, z (uT)
    float trinvW[3][3];          // trial inverse soft iron matrix size
    float trB;                   // trial value of geomagnetic field magnitude in uT
    float trFitErrorpc;          // trial value of fit error %
    float A[3][3];               // ellipsoid matrix A
    float invA[3][3];            // inverse of ellipsoid matrix A
    float matA[10][10];          // scratch 10x10 matrix used by calibration algorithms
    float matB[10][10];          // scratch 10x10 matrix used by calibration algorithms
    float vecA[10];              // scratch 10x1 vector used by calibration algorithms
    float vecB[4];               // scratch 4x1 vector used by calibration algorithms
    int8_t ValidMagCal;          // integer value 0, 4, 7, 10 denoting both valid calibration and solver used
    int16_t BpFast[3][MAGBUFFSIZE];   // uncalibrated magnetometer readings
    int8_t  valid[MAGBUFFSIZE];        // 1=has data, 0=empty slot
    int16_t MagBufferCount;           // number of magnetometer readings
} MagCalibration_t;

extern MagCalibration_t magcal;


void f3x3matrixAeqI(float A[][3]);
void fmatrixAeqI(float *A[], int16_t rc);
void f3x3matrixAeqScalar(float A[][3], float Scalar);
void f3x3matrixAeqInvSymB(float A[][3], float B[][3]);
void f3x3matrixAeqAxScalar(float A[][3], float Scalar);
void f3x3matrixAeqMinusA(float A[][3]);
float f3x3matrixDetA(float A[][3]);
void eigencompute(float A[][10], float eigval[], float eigvec[][10], int8_t n);
void fmatrixAeqInvA(float *A[], int8_t iColInd[], int8_t iRowInd[], int8_t iPivot[], int8_t isize);
void fmatrixAeqRenormRotA(float A[][3]);


#define SENSORFS 100
#define OVERSAMPLE_RATIO 4


// accelerometer sensor structure definition
#define G_PER_COUNT 0.0001220703125F  // = 1/8192
typedef struct
{
	//int32_t iSumGpFast[3];    // sum of fast measurements
	float GpFast[3];       // fast (typically 200Hz) readings (g)
	float Gp[3];           // slow (typically 25Hz) averaged readings (g)
	//float fgPerCount;       // initialized to FGPERCOUNT
	//int16_t iGpFast[3];       // fast (typically 200Hz) readings
	//int16_t iGp[3];           // slow (typically 25Hz) averaged readings (counts)
} AccelSensor_t;

// magnetometer sensor structure definition
#define UT_PER_COUNT 0.1F
typedef struct
{
	//int32_t iSumBpFast[3];    // sum of fast measurements
	//float fBpFast[3];       // fast (typically 200Hz) raw readings (uT)
	//float fBp[3];           // slow (typically 25Hz) averaged raw readings (uT)
	float BcFast[3];       // fast (typically 200Hz) calibrated readings (uT)
	float Bc[3];           // slow (typically 25Hz) averaged calibrated readings (uT)
	//float fuTPerCount;      // initialized to FUTPERCOUNT
	//float fCountsPeruT;     // initialized to FCOUNTSPERUT
	//int16_t iBpFast[3];       // fast (typically 200Hz) raw readings (counts)
	//int16_t iBp[3];           // slow (typically 25Hz) averaged raw readings (counts)
	//int16_t iBc[3];           // slow (typically 25Hz) averaged calibrated readings (counts)
} MagSensor_t;

// gyro sensor structure definition
#define DEG_PER_SEC_PER_COUNT 0.0625F  // = 1/16
typedef struct
{
	//int32_t iSumYpFast[3];                    // sum of fast measurements
	float Yp[3];                           // raw gyro sensor output (deg/s)
	//float fDegPerSecPerCount;               // initialized to FDEGPERSECPERCOUNT
	int16_t YpFast[OVERSAMPLE_RATIO][3];     // fast (typically 200Hz) readings
	//int16_t iYp[3];                           // averaged gyro sensor output (counts)
} GyroSensor_t;



// 9DOF Kalman filter accelerometer, magnetometer and gyroscope state vector structure
typedef struct
{
	// start: elements common to all motion state vectors
	// Euler angles
	float PhiPl;			// roll (deg)
	float ThePl;			// pitch (deg)
	float PsiPl;			// yaw (deg)
	float RhoPl;			// compass (deg)
	float ChiPl;			// tilt from vertical (deg)
	// orientation matrix, quaternion and rotation vector
	float RPl[3][3];		// a posteriori orientation matrix
	Quaternion_t qPl;		// a posteriori orientation quaternion
	float RVecPl[3];		// rotation vector
	// angular velocity
	float Omega[3];			// angular velocity (deg/s)
	// systick timer for benchmarking
	int32_t systick;		// systick timer;
	// end: elements common to all motion state vectors

	// elements transmitted over bluetooth in kalman packet
	float bPl[3];			// gyro offset (deg/s)
	float ThErrPl[3];		// orientation error (deg)
	float bErrPl[3];		// gyro offset error (deg/s)
	// end elements transmitted in kalman packet

	float dErrGlPl[3];		// magnetic disturbance error (uT, global frame)
	float dErrSePl[3];		// magnetic disturbance error (uT, sensor frame)
	float aErrSePl[3];		// linear acceleration error (g, sensor frame)
	float aSeMi[3];			// linear acceleration (g, sensor frame)
	float DeltaPl;			// inclination angle (deg)
	float aSePl[3];			// linear acceleration (g, sensor frame)
	float aGlPl[3];			// linear acceleration (g, global frame)
	float gErrSeMi[3];		// difference (g, sensor frame) of gravity vector (accel) and gravity vector (gyro)
	float mErrSeMi[3];		// difference (uT, sensor frame) of geomagnetic vector (magnetometer) and geomagnetic vector (gyro)
	float gSeGyMi[3];		// gravity vector (g, sensor frame) measurement from gyro
	float mSeGyMi[3];		// geomagnetic vector (uT, sensor frame) measurement from gyro
	float mGl[3];			// geomagnetic vector (uT, global frame)
	float QvAA;			// accelerometer terms of Qv
	float QvMM;			// magnetometer terms of Qv
	float PPlus12x12[12][12];	// covariance matrix P+
	float K12x6[12][6];		// kalman filter gain matrix K
	float Qw12x12[12][12];		// covariance matrix Qw
	float C6x12[6][12];		// measurement matrix C
	float RMi[3][3];		// a priori orientation matrix
	Quaternion_t Deltaq;		// delta quaternion
	Quaternion_t qMi;		// a priori orientation quaternion
	float casq;			// FCA * FCA;
	float cdsq;			// FCD * FCD;
	float Fastdeltat;		// sensor sampling interval (s) = 1 / SENSORFS
	float deltat;			// kalman filter sampling interval (s) = OVERSAMPLE_RATIO / SENSORFS
	float deltatsq;			// fdeltat * fdeltat
	float QwbplusQvG;		// FQWB + FQVG
	int16_t FirstOrientationLock;	// denotes that 9DOF orientation has locked to 6DOF
	int8_t resetflag;		// flag to request re-initialization on next pass
} SV_9DOF_GBY_KALMAN_t;

void fInit_9DOF_GBY_KALMAN(SV_9DOF_GBY_KALMAN_t *SV);
void fRun_9DOF_GBY_KALMAN(SV_9DOF_GBY_KALMAN_t *SV,
	const AccelSensor_t *Accel, const MagSensor_t *Mag, const GyroSensor_t *Gyro,
	const MagCalibration_t *MagCal);



#ifdef __cplusplus
} // extern "C"
#endif

#endif
