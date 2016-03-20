#include "imuread.h"


// longitude = 0 to 2pi  (meaning 0 to 360 degrees)
// latitude = -pi/2 to +pi/2  (meaning -90 to +90 degrees)
// return 0 to 99 - which region on the sphere (100 of equal surface area)
static int sphere_region(float longitude, float latitude)
{
	int region;

	// https://etna.mcs.kent.edu/vol.25.2006/pp309-327.dir/pp309-327.html
	// sphere equations....
	//  area of unit sphere = 4*pi
	//  area of unit sphere cap = 2*pi*h  h = cap height
	//  lattitude of unit sphere cap = arcsin(1 - h)
	if (latitude > 1.37046f /* 78.52 deg */) {
		// arctic cap, 1 region
		return 0;
	} else if (latitude < -1.37046f /* -78.52 deg */) {
		// antarctic cap, 1 region
		return 99;
	} else if (latitude > 0.74776f /* 42.84 deg */ || latitude < -0.74776f ) {
		// temperate zones, 15 regions each
		region = floorf(longitude * (float)(15.0 / (M_PI * 2.0)));
		if (region < 0) region = 0;
		else if (region > 14) region = 14;
		if (latitude > 0.0) {
			return region + 1; // 1 to 15
		} else {
			return region + 84; // 84 to 98
		}
	} else {
		// tropic zones, 34 regions each
		region = floorf(longitude * (float)(34.0 / (M_PI * 2.0)));
		if (region < 0) region = 0;
		else if (region > 33) region = 33;
		if (latitude >= 0.0) {
			return region + 16; // 16 to 49
		} else {
			return region + 50; // 50 to 83
		}
	}
}


static int count=0;
static int spheredist[100];
static float magnitude[MAGBUFFSIZE];

void quality_reset(void)
{
	count=0;
	memset(spheredist, 0, sizeof(spheredist));
}

void quality_update(const Point_t *point)
{
	float x, y, z;
	float latitude, longitude;

	x = point->x;
	y = point->y;
	z = point->z;
	magnitude[count] = sqrtf(x * x + y * y + z * z);
	longitude = atan2f(y, x) + (float)M_PI;
	latitude = atan2f(sqrtf(x * x + y * y), z) - (float)(M_PI / 2.0);
	spheredist[sphere_region(longitude, latitude)]++;
	count++;
}

// How many surface gaps
float quality_surface_gap_error(void)
{
	float error=0.0f;
	int i, num;

	for (i=0; i < 100; i++) {
		num = spheredist[i];
		if (num == 0) {
			error += 1.0f;
		} else if (num == 1) {
			error += 0.2f;
		} else if (num == 2) {
			error += 0.01f;
		}
	}
	return error;
}

// Variance in magnitude
float quality_magnitude_variance_error(void)
{
	float sum, mean, diff, variance;
	int i;

	sum = 0.0f;
	for (i=0; i < count; i++) {
		sum += magnitude[i];
	}
	mean = sum / (float)count;
	variance = 0.0f;
	for (i=0; i < count; i++) {
		diff = magnitude[i] - mean;
		variance += diff * diff;
	}
	variance /= (float)count;
	return sqrtf(variance) / mean * 100.0f;
}

// Offset of centroid
float quality_centroid_placement_error(void)
{
	return 0.0f;
}

// Freescale's algorithm fit error
float quality_spherical_fit_error(void)
{
	return magcal.FitError;
}


