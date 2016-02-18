#include "imuread.h"
#include <GL/glut.h> // sudo apt-get install xorg-dev libglu1-mesa-dev freeglut3-dev

static void timer_callback(int val)
{
	glutTimerFunc(TIMEOUT_MSEC, timer_callback, 0);
	read_serial_data();
	glutPostRedisplay(); // TODO: only redisplay if data changes
}

static void glut_display_callback(void)
{
	display_callback();
	glutSwapBuffers();
}

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(600, 500);
	glutCreateWindow("IMU Read");

	visualize_init();

	glutReshapeFunc(resize_callback);
	glutDisplayFunc(glut_display_callback);
	glutTimerFunc(TIMEOUT_MSEC, timer_callback, 0);

	open_port(PORT);
	glutMainLoop();
	close_port();
	return 0;
}



