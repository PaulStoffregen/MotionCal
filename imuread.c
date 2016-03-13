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

static void glut_keystroke_callback(unsigned char ch, int x, int y)
{
	if (magcal.fFitErrorpc > 9.0) {
		printf("Poor Calibration: ");
		printf("soft iron fit error = %.1f%%\n", magcal.fFitErrorpc);
		return;
	}
	printf("Magnetic Calibration:   (%.1f%% fit error)\n", magcal.fFitErrorpc);
	printf("   %7.2f   %6.3f %6.3f %6.3f\n",
		magcal.fV[0], magcal.finvW[0][0], magcal.finvW[0][1], magcal.finvW[0][2]);
	printf("   %7.2f   %6.3f %6.3f %6.3f\n",
		magcal.fV[1], magcal.finvW[1][0], magcal.finvW[1][1], magcal.finvW[1][2]);
	printf("   %7.2f   %6.3f %6.3f %6.3f\n",
		magcal.fV[2], magcal.finvW[2][0], magcal.finvW[2][1], magcal.finvW[2][2]);
	send_calibration();
}

int main(int argc, char *argv[])
{
	raw_data_reset();

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(600, 500);
	glutCreateWindow("IMU Read");

	visualize_init();

	glutReshapeFunc(resize_callback);
	glutDisplayFunc(glut_display_callback);
	glutTimerFunc(TIMEOUT_MSEC, timer_callback, 0);
	glutKeyboardFunc(glut_keystroke_callback);

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
