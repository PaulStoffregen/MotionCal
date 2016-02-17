#include "imuread.h"

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

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(600, 500);
	glutCreateWindow("IMU Read");

	visualize_init();

	glutReshapeFunc(resize_callback);
	glutDisplayFunc(display_callback);
	glutTimerFunc(TIMEOUT_MSEC, timer_callback, 0);

	open_port(PORT);
	glutMainLoop();
	close_port();
	return 0;
}



