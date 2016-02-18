#ifndef IMUread_h_
#define IMUread_h_

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
#include <GL/gl.h>
#include <GL/glu.h>

#define PORT "/dev/ttyACM0"
#define TIMEOUT_MSEC 40

#define MAGBUFFSIZEX 14
#define MAGBUFFSIZEY 28
#define MAGBUFFSIZE (MAGBUFFSIZEX * MAGBUFFSIZEY)

#ifdef __cplusplus
extern "C"{
#endif

typedef struct {
	float x;
	float y;
	float z;
	int valid;
} magdata_t;
extern magdata_t caldata[MAGBUFFSIZE];
extern magdata_t hard_iron;
extern magdata_t current_position;
typedef struct {
	float w;
	float x;
	float y;
	float z;
} quat_t;
extern quat_t current_orientation;

extern void die(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
extern void *malloc_or_die(size_t size);
extern int open_port(const char *name);
extern void read_serial_data(void);
extern void close_port(void);
void visualize_init(void);
void display_callback(void);
void resize_callback(int width, int height);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
