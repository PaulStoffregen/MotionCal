#if defined(LINUX)

#include "imuread.h"
#include <GL/glut.h> // sudo apt-get install xorg-dev libglu1-mesa-dev freeglut3-dev

void die(const char *format, ...) __attribute__ ((format (printf, 1, 2)));

static void timer_callback(int val)
{
	int r;

	glutTimerFunc(TIMEOUT_MSEC, timer_callback, 0);
	r = read_serial_data();
	if (r < 0) die("Error reading %s\n", PORT);
	glutPostRedisplay(); // TODO: only redisplay if data changes
}

static void glut_display_callback(void)
{
	display_callback();
	glutSwapBuffers();
}

int main(int argc, char *argv[])
{
	memset(&magcal, 0, sizeof(magcal));
	magcal.fV[2] = 80.0f;
	magcal.finvW[0][0] = 1.0f;
	magcal.finvW[1][1] = 1.0f;
	magcal.finvW[2][2] = 1.0f;

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(600, 500);
	glutCreateWindow("IMU Read");

	visualize_init();

	glutReshapeFunc(resize_callback);
	glutDisplayFunc(glut_display_callback);
	glutTimerFunc(TIMEOUT_MSEC, timer_callback, 0);

	if (!open_port(PORT)) die("Unable to open %s\n", PORT);
	glutMainLoop();
	close_port();
	return 0;
}

void die(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	close_port();
	exit(1);
}

#else

int main(int argc, char *argv[])
{
	return 0;
}

#endif
