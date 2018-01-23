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

extern int invert_q0;
extern int invert_q1;
extern int invert_q2;
extern int invert_q3;
extern int invert_x;
extern int invert_y;
extern int invert_z;

static void print_invert_state(void)
{
	printf("Invert: %s %s %s %s  %s %s %s\n",
		(invert_q0 ? "Q0" : "  "),
		(invert_q1 ? "Q1" : "  "),
		(invert_q2 ? "Q2" : "  "),
		(invert_q3 ? "Q3" : "  "),
		(invert_x ? "x'" : "  "),
		(invert_y ? "y'" : "  "),
		(invert_z ? "z'" : "  ")
	);
}


static void glut_keystroke_callback(unsigned char ch, int x, int y)
{
	if (ch == '0') {
		invert_q0 ^= 1;
		print_invert_state();
		return;
	}
	if (ch == '1') {
		invert_q1 ^= 1;
		print_invert_state();
		return;
	}
	if (ch == '2') {
		invert_q2 ^= 1;
		print_invert_state();
		return;
	}
	if (ch == '3') {
		invert_q3 ^= 1;
		print_invert_state();
		return;
	}
	if (ch == 'x') {
		invert_x ^= 1;
		print_invert_state();
		return;
	}
	if (ch == 'y') {
		invert_y ^= 1;
		print_invert_state();
		return;
	}
	if (ch == 'z') {
		invert_z ^= 1;
		print_invert_state();
		return;
	}


	if (magcal.FitError > 9.0) {
		printf("Poor Calibration: ");
		printf("soft iron fit error = %.1f%%\n", magcal.FitError);
		return;
	}
	printf("Magnetic Calibration:   (%.1f%% fit error)\n", magcal.FitError);
	printf("   %7.2f   %6.3f %6.3f %6.3f\n",
		magcal.V[0], magcal.invW[0][0], magcal.invW[0][1], magcal.invW[0][2]);
	printf("   %7.2f   %6.3f %6.3f %6.3f\n",
		magcal.V[1], magcal.invW[1][0], magcal.invW[1][1], magcal.invW[1][2]);
	printf("   %7.2f   %6.3f %6.3f %6.3f\n",
		magcal.V[2], magcal.invW[2][0], magcal.invW[2][1], magcal.invW[2][2]);
	send_calibration();
}

void calibration_confirmed(void)
{
	printf("Calibration confirmed!\n");
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
