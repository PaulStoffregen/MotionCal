#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>

#define PORT "/dev/ttyACM0"

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


void packet(const unsigned char *data, int len)
{
	print_data("packet", data, len);

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


int main()
{
	int fd, r;
	struct termios termsettings;
	struct timeval tv;
	fd_set rdfs;
	unsigned char buf[256];

	fd = open(PORT, O_RDWR | O_NONBLOCK);
	if (fd < 0) die("Unable to open %s\n", PORT);

	r = tcgetattr(fd, &termsettings);
	if (r < 0) die("Unable to read terminal settings from %s\n", PORT);
	cfmakeraw(&termsettings);
	cfsetspeed(&termsettings, B115200);
	r = tcsetattr(fd, TCSANOW, &termsettings);
	if (r < 0) die("Unable to program terminal settings on %s\n", PORT);

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

