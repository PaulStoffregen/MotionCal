#include "imuread.h"

MagCalibration_t magcal;

Quaternion_t current_orientation;

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


static void apply_calibration(int16_t rawx, int16_t rawy, int16_t rawz, Point_t *out)
{
	float x, y, z;

	x = ((float)rawx * 0.1) - magcal.fV[0];
	y = ((float)rawy * 0.1) - magcal.fV[1];
	z = ((float)rawz * 0.1) - magcal.fV[2];
	out->x = x * magcal.finvW[0][0] + y * magcal.finvW[0][1] + z * magcal.finvW[0][2];
	out->y = x * magcal.finvW[1][0] + y * magcal.finvW[1][1] + z * magcal.finvW[1][2];
	out->z = x * magcal.finvW[2][0] + y * magcal.finvW[2][1] + z * magcal.finvW[2][2];
	out->valid = 1;
}

static void quad_to_rotation(const Quaternion_t *quat, float *rmatrix)
{
	float qw = quat->q0;
	float qx = quat->q1;
	float qy = quat->q2;
	float qz = quat->q3;
	rmatrix[0] = 1.0f - 2.0f * qy * qy - 2.0f * qz * qz;
	rmatrix[1] = 2.0f * qx * qy - 2.0f * qz * qw;
	rmatrix[2] = 2.0f * qx * qz + 2.0f * qy * qw;
	rmatrix[3] = 2.0f * qx * qy + 2.0f * qz * qw;
	rmatrix[4] = 1.0f  - 2.0f * qx * qx - 2.0f * qz * qz;
	rmatrix[5] = 2.0f * qy * qz - 2.0f * qx * qw;
	rmatrix[6] = 2.0f * qx * qz - 2.0f * qy * qw;
	rmatrix[7] = 2.0f * qy * qz + 2.0f * qx * qw;
	rmatrix[8] = 1.0f  - 2.0f * qx * qx - 2.0f * qy * qy;
}

static void rotate(const Point_t *in, Point_t *out, const float *rmatrix)
{
	if (out == NULL) return;
	if (in == NULL || in->valid == 0) {
		out->valid = 0;
		out->x = out->y = out->z = 0;
		return;
	}
	out->x = in->x * rmatrix[0] + in->y * rmatrix[1] + in->z * rmatrix[2];
	out->y = in->x * rmatrix[3] + in->y * rmatrix[4] + in->z * rmatrix[5];
	out->z = in->x * rmatrix[6] + in->y * rmatrix[7] + in->z * rmatrix[8];
}



static GLuint spherelist;
static GLuint spherelowreslist;

void display_callback(void)
{
	//static int updatenum=0;
	int i, count=0;
	float xscale, yscale, zscale;
	float xoff, yoff, zoff;
	float rotation[9];
	Point_t point, draw;
	float magnitude[MAGBUFFSIZE];
	float latitude[MAGBUFFSIZE]; // 0 to PI
	float longitude[MAGBUFFSIZE]; // -PI to +PI
	float sumx=0.0, sumy=0.0, sumz=0.0, summag=0.0;
	int spheredist[100];

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glColor3f(1, 0, 0);	// set current color to red
	memset(spheredist, 0, sizeof(spheredist));

	xscale = 0.05;
	yscale = 0.05;
	zscale = 0.05;
	xoff = 0.0;
	yoff = 0.0;
	zoff = -7.0;

	//if (hard_iron.valid) {
	if (1) {
		quad_to_rotation(&current_orientation, rotation);
		for (i=0; i < MAGBUFFSIZE; i++) {
			//if (caldata[i].valid) {
			if (magcal.valid[i]) {
				//apply_calibration(&caldata[i], &point);
				apply_calibration(magcal.iBpFast[0][i], magcal.iBpFast[1][i],
					magcal.iBpFast[2][i], &point);
				sumx += point.x;
				sumy += point.y;
				sumz += point.z;
				magnitude[i] = sqrtf(point.x * point.x +
					point.y * point.y + point.z * point.z);
				summag += magnitude[i];
				longitude[i] = atan2f(point.y, point.x) + (float)M_PI;
				latitude[i] = atan2f(sqrtf(point.x * point.x +
					point.y * point.y), point.z) - (float)(M_PI / 2.0);
				spheredist[sphere_region(longitude[i], latitude[i])]++;
				count++;
				rotate(&point, &draw, rotation);
				glPushMatrix();
				glTranslatef(
					draw.x * xscale + xoff,
					draw.z * yscale + yoff,
					draw.y * zscale + zoff
				);
				if (draw.y >= 0.0f) {
					glCallList(spherelist);
				} else {
					glCallList(spherelowreslist);
				}
				glPopMatrix();
			}
		}
	}
#if 0
	if (updatenum++ == 500) {
		for (i=0; i < MAGBUFFSIZE; i++) {
			if (caldata[i].valid) {
				printf("long: %6.2f  lat: %6.2f (%.2f)\n",
					longitude[i] * 180.0 / M_PI,
					latitude[i] * 180.0 / M_PI, latitude[i]);
			}
		}
		for (i=0; i < 100; i++) {
			printf("sphere %d region: %d points\n", 1, spheredist[i]);
		}
		printf("count = %d\n", count);
		printf("sum = %.2f %.2f %.2f\n", sumx, sumy, sumz);
		printf("summag = %.2f  avg: %.2f\n", summag, summag / (float)count);
		printf("exit here\n");
		exit(1);
	}
#endif
}

void resize_callback(int width, int height)
{
	const float ar = (float) width / (float) height;

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-ar, ar, -1.0, 1.0, 2.0, 100.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity() ;
}


static const GLfloat light_ambient[]  = { 0.0f, 0.0f, 0.0f, 1.0f };
static const GLfloat light_diffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
static const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
static const GLfloat light_position[] = { 2.0f, 5.0f, 5.0f, 0.0f };

static const GLfloat mat_ambient[]    = { 0.7f, 0.7f, 0.7f, 1.0f };
static const GLfloat mat_diffuse[]    = { 0.8f, 0.8f, 0.8f, 1.0f };
static const GLfloat mat_specular[]   = { 1.0f, 1.0f, 1.0f, 1.0f };
static const GLfloat high_shininess[] = { 100.0f };

void visualize_init(void)
{
	GLUquadric *sphere;

	glClearColor(1.0, 1.0, 1.0, 1.0);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	//glShadeModel(GL_FLAT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_LIGHT0);
	//glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glMaterialfv(GL_FRONT, GL_AMBIENT,   mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE,   mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR,  mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);

	sphere = gluNewQuadric();
	gluQuadricDrawStyle(sphere, GLU_FILL);
	gluQuadricNormals(sphere, GLU_SMOOTH);
	spherelist = glGenLists(1);
	glNewList(spherelist, GL_COMPILE);
	gluSphere(sphere, 0.08, 16, 14);
	glEndList();
	spherelowreslist = glGenLists(1);
	glNewList(spherelowreslist, GL_COMPILE);
	gluSphere(sphere, 0.08, 12, 10);
	glEndList();
	gluDeleteQuadric(sphere);
}



