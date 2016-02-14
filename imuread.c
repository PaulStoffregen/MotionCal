#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <GL/glut.h> // sudo apt-get install xorg-dev libglu1-mesa-dev freeglut3-dev

#define PORT "/dev/ttyACM0"
#define TIMEOUT_MSEC 40


#define MAGBUFFSIZEX 14
#define MAGBUFFSIZEY 28
#define MAGBUFFSIZE (MAGBUFFSIZEX * MAGBUFFSIZEY)

typedef struct {
	float x;
	float y;
	float z;
	int valid;
} magdata_t;
magdata_t caldata[MAGBUFFSIZE];
magdata_t hard_iron;
magdata_t current_position;
typedef struct {
	float q1;
	float q2;
	float q3;
	float q4;
} quat_t;
quat_t current_orientation;

int fd=-1;
void die(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
void *malloc_or_die(size_t size);

void print_data(const char *name, const unsigned char *data, int len)
{
	int i;

	printf("%s (%2d bytes):", name, len);
	for (i=0; i < len; i++) {
		printf(" %02X", data[i]);
	}
	printf("\n");
}


void packet_primary_data(const unsigned char *data)
{
	int16_t x, y, z, w;

	x = (data[13] << 8) | data[12];
	y = (data[15] << 8) | data[14];
	z = (data[17] << 8) | data[16];
	current_position.x = (float)x / 10.0f;
	current_position.y = (float)y / 10.0f;
	current_position.z = (float)z / 10.0f;
	current_orientation.q1 = (float)((int16_t)((data[25] << 8) | data[24])) / 30000.0f;
	current_orientation.q2 = (float)((int16_t)((data[27] << 8) | data[26])) / 30000.0f;
	current_orientation.q3 = (float)((int16_t)((data[29] << 8) | data[28])) / 30000.0f;
	current_orientation.q4 = (float)((int16_t)((data[31] << 8) | data[30])) / 30000.0f;
#if 0
	printf("mag data, %5.2f %5.2f %5.2f\n",
		current_position.x,
		current_position.y,
		current_position.z
	);
#endif
#if 0
	printf("orientation: %5.3f %5.3f %5.3f %5.3f\n",
		current_orientation.q1,
		current_orientation.q2,
		current_orientation.q3,
		current_orientation.q4
	);
#endif
}

void packet_magnetic_cal(const unsigned char *data)
{
	int16_t id, x, y, z;
	magdata_t *cal;
	float newx, newy, newz;

	id = (data[7] << 8) | data[6];
	x = (data[9] << 8) | data[8];
	y = (data[11] << 8) | data[10];
	z = (data[13] << 8) | data[12];

	if (id == 1) {
		cal = &hard_iron;
		cal->x = (float)x / 10.0f;
		cal->y = (float)y / 10.0f;
		cal->z = (float)z / 10.0f;
		cal->valid = 1;
	} else if (id >= 10 && id < MAGBUFFSIZE+10) {
		newx = (float)x / 10.0f;
		newy = (float)y / 10.0f;
		newz = (float)z / 10.0f;
		cal = &caldata[id - 10];
		if (!cal->valid || cal->x != newx || cal->y != newy || cal->z != newz) {
			cal->x = newx;
			cal->y = newy;
			cal->z = newz;
			cal->valid = 1;
			printf("mag cal, id=%3d: %5d %5d %5d\n", id, x, y, z);
		}
	}
}

void packet(const unsigned char *data, int len)
{
	if (len <= 0) return;
	//print_data("packet", data, len);

	if (data[0] == 1 && len == 34) {
		packet_primary_data(data);
	} else if (data[0] == 6 && len == 14) {
		packet_magnetic_cal(data);
	}
	// TODO: actually do something with the arriving data...
}

void packet_encoded(const unsigned char *data, int len)
{
	const unsigned char *p;
	unsigned char buf[256];
	int buflen=0, copylen;

	//printf("packet_encoded, len = %d\n", len);
	p = memchr(data, 0x7D, len);
	if (p == NULL) {
		packet(data, len);
	} else {
		//printf("** decoding necessary\n");
		while (1) {
			copylen = p - data;
			if (copylen > 0) {
				//printf("  copylen = %d\n", copylen);
				// TODO: overflow check
				memcpy(buf+buflen, data, copylen);
				buflen += copylen;
				data += copylen;
				len -= copylen;
			}
			// TODO: overflow check
			buf[buflen++] = (p[1] == 0x5E) ? 0x7E : 0x7D;
			data += 2;
			len -= 2;
			if (len <= 0) break;
			p = memchr(data, 0x7D, len);
			if (p == NULL) {
				// TODO: overflow check
				memcpy(buf+buflen, data, len);
				buflen += len;
				break;
			}
		}
		//printf("** decoded to %d\n", buflen);
		packet(buf, buflen);
	}
}

void newdata(const unsigned char *data, int len)
{
	static unsigned char packetbuf[256];
	static unsigned int packetlen=0;
	const unsigned char *p;
	int copylen;

	//print_data("newdata", data, len);
	while (len > 0) {
		p = memchr(data, 0x7E, len);
		if (p == NULL) {
			// TODO: overflow check
			memcpy(packetbuf+packetlen, data, len);
			packetlen += len;
			len = 0;
		} else if (p > data) {
			copylen = p - data;
			// TODO: overflow check
			memcpy(packetbuf+packetlen, data, copylen);
			packet_encoded(packetbuf, packetlen+copylen);
			packetlen = 0;
			data += copylen + 1;
			len -= copylen + 1;
		} else {
			if (packetlen > 0) {
				packet_encoded(packetbuf, packetlen);
				packetlen = 0;
			}
			data++;
			len--;
		}
	}
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
	float x, y, z;
	float xscale, yscale, zscale;
	float xoff, yoff, zoff;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glColor3f(1, 0, 0);	// set current color to black
#if 0
	glBegin(GL_POLYGON);		// draw filled triangle
	glVertex2i(200, 125);		// specify each vertex of triangle
	glVertex2i(100, 375);
	glVertex2i(300, 375);
	glEnd();			// OpenGL draws the filled triangle
#endif

	xscale = 0.05;
	yscale = 0.05;
	zscale = 0.05;
	xoff = 0.0;
	yoff = 0.0;
	zoff = -7.0;

	if (hard_iron.valid) {
		for (i=0; i < MAGBUFFSIZE; i++) {
			if (caldata[i].valid) {
				x = caldata[i].x - hard_iron.x;
				y = caldata[i].y - hard_iron.y;
				z = caldata[i].z - hard_iron.z;
				glPushMatrix();
				glTranslatef(
					x * xscale + xoff,
					y * yscale + yoff,
					z * zscale + zoff
				);
				glutSolidSphere(0.08,16,16); // radius, slices, stacks
				glPopMatrix();
			}
		}
	}
	//glTranslated(0.0,1.2,-30);
	//glutSolidSphere(1,20,20); // radius, slices, stacks
	//glutWireSphere(3,20,20); // radius, slices, stacks


	glutSwapBuffers();
}



void timer_callback(int val)
{
	unsigned char buf[256];
	static int nodata_count=0;
	int n;

	//printf("timer_callback\n");
	glutTimerFunc(TIMEOUT_MSEC, timer_callback, 0);
	while (fd >= 0) {
		n = read(fd, buf, sizeof(buf));
		if (n > 0 && n <= sizeof(buf)) {
			//printf("read %d bytes\n", r);
			newdata(buf, n);
			nodata_count = 0;
		} else if (n == 0) {
			printf("read no data\n");
			if (++nodata_count > 6) {
				close(fd);
				fd = -1;
				die("No data from %s\n", PORT);
			}
			break;
		} else {
			n = errno;
			if (n == EAGAIN) {
				break;
			} else if (n == EAGAIN) {
			} else {
				printf("read error, n=%d\n", n);
				break;
			}
		}
	}
	glutPostRedisplay();
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
	int r;
	struct termios termsettings;
	//struct timeval tv;
	//fd_set rdfs;
	//unsigned char buf[256];

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

	fd = open(PORT, O_RDWR | O_NONBLOCK);
	if (fd < 0) die("Unable to open %s\n", PORT);

	r = tcgetattr(fd, &termsettings);
	if (r < 0) die("Unable to read terminal settings from %s\n", PORT);
	cfmakeraw(&termsettings);
	cfsetspeed(&termsettings, B115200);
	r = tcsetattr(fd, TCSANOW, &termsettings);
	if (r < 0) die("Unable to program terminal settings on %s\n", PORT);


	glutMainLoop();

#if 0
	// GLUT lacks any way to register I/O callbacks
	// so we'll just have to poll from a timer
	while (1) {
		FD_ZERO(&rdfs);
		FD_SET(fd, &rdfs);
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		r = select(fd+1, &rdfs, NULL, NULL, &tv);
		if (r == 1) {
			// data
			r = read(fd, buf, sizeof(buf));
			if (r > 0 && r <= sizeof(buf)) {
				//printf("read %d bytes\n", r);
				newdata(buf, r);
			} else if (r == 0) {
				printf("read no data\n");
				break;
			} else {
				printf("read error\n");
				break;
			}
		} else if (r == 0) {
			printf("timeout\n");
			// timeout
		} else {
			printf("select error\n");
			break;
		}
	}
#endif
	close(fd);
	return 0;
}




void die(const char *format, ...)
{
        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        exit(1);
}

void *malloc_or_die(size_t size)
{
        void *p;

        p = malloc(size);
        if (!p) die("unable to allocate memory, %d bytes\n", (int)size);
        return p;
}

