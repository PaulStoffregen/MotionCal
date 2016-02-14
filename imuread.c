#include "imuread.h"

magdata_t caldata[MAGBUFFSIZE];
magdata_t hard_iron;
magdata_t current_position;
quat_t current_orientation;


void quad_to_rotation(const quat_t *quat, float *rmatrix)
{
	float qx = quat->x;
	float qy = quat->y;
	float qz = quat->z;
	float qw = quat->w;

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

void rotate(const magdata_t *in, magdata_t *out, const float *rmatrix)
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
	out->valid = 1;
}


/*
typedef struct {
	float x;
	float y;
	float z;
	int valid;
} magdata_t;
magdata_t caldata[MAGBUFFSIZE];
*/

void display_callback(void)
{
	int i;
	//float x, y, z;
	float xscale, yscale, zscale;
	float xoff, yoff, zoff;
	float rotation[9];
	magdata_t point, draw;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glColor3f(1, 0, 0);	// set current color to red

	xscale = 0.05;
	yscale = 0.05;
	zscale = 0.05;
	xoff = 0.0;
	yoff = 0.0;
	zoff = -7.0;

	if (hard_iron.valid) {
		quad_to_rotation(&current_orientation, rotation);
		for (i=0; i < MAGBUFFSIZE; i++) {
			if (caldata[i].valid) {
				point.x = caldata[i].x - hard_iron.x;
				point.y = caldata[i].y - hard_iron.y;
				point.z = caldata[i].z - hard_iron.z;
				point.valid = 1;
				// TODO: apply soft iron cal

				memcpy(&draw, &point, sizeof(draw));
				rotate(&point, &draw, rotation);

				glPushMatrix();
				glTranslatef(
					draw.x * xscale + xoff,
					draw.z * yscale + yoff,
					draw.y * zscale + zoff
				);
				glutSolidSphere(0.08,16,16); // radius, slices, stacks
				glPopMatrix();
			}
		}
	}
	glutSwapBuffers();
}

void timer_callback(int val)
{
	glutTimerFunc(TIMEOUT_MSEC, timer_callback, 0);
	read_serial_data();
	glutPostRedisplay(); // TODO: only redisplay if data changes
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

const GLfloat light_ambient[]  = { 0.0f, 0.0f, 0.0f, 1.0f };
const GLfloat light_diffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_position[] = { 2.0f, 5.0f, 5.0f, 0.0f };

const GLfloat mat_ambient[]    = { 0.7f, 0.7f, 0.7f, 1.0f };
const GLfloat mat_diffuse[]    = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_specular[]   = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat high_shininess[] = { 100.0f };

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(600, 500);
	glutCreateWindow("IMU Read");
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	//glShadeModel(GL_FLAT);
	//gluOrtho2D(0, 600, 0, 500);
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

	glutReshapeFunc(resize_callback);
	glutDisplayFunc(display_callback);
	glutTimerFunc(TIMEOUT_MSEC, timer_callback, 0);

	open_port(PORT);
	glutMainLoop();
	close_port();
	return 0;
}



