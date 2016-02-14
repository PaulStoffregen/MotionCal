#include "imuread.h"

static int portfd=-1;

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
	current_position.x = (float)((int16_t)((data[13] << 8) | data[12])) / 10.0f;
	current_position.y = (float)((int16_t)((data[15] << 8) | data[14])) / 10.0f;
	current_position.z = (float)((int16_t)((data[17] << 8) | data[16])) / 10.0f;
	current_orientation.w = (float)((int16_t)((data[25] << 8) | data[24])) / 30000.0f;
	current_orientation.x = (float)((int16_t)((data[27] << 8) | data[26])) / 30000.0f;
	current_orientation.y = (float)((int16_t)((data[29] << 8) | data[28])) / 30000.0f;
	current_orientation.z = (float)((int16_t)((data[31] << 8) | data[30])) / 30000.0f;
#if 0
	printf("mag data, %5.2f %5.2f %5.2f\n",
		current_position.x,
		current_position.y,
		current_position.z
	);
#endif
#if 0
	printf("orientation: %5.3f %5.3f %5.3f %5.3f\n",
		current_orientation.w,
		current_orientation.x,
		current_orientation.y,
		current_orientation.z
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

int open_port(const char *name)
{
	struct termios termsettings;
	int r;

	portfd = open(name, O_RDWR | O_NONBLOCK);
	if (portfd < 0) die("Unable to open %s\n", name);

	r = tcgetattr(portfd, &termsettings);
	if (r < 0) die("Unable to read terminal settings from %s\n", name);
	cfmakeraw(&termsettings);
	cfsetspeed(&termsettings, B115200);
	r = tcsetattr(portfd, TCSANOW, &termsettings);
	if (r < 0) die("Unable to program terminal settings on %s\n", name);
	return 0;
}

void read_serial_data(void)
{
	unsigned char buf[256];
	static int nodata_count=0;
	int n;

	while (portfd >= 0) {
		n = read(portfd, buf, sizeof(buf));
		if (n > 0 && n <= sizeof(buf)) {
			//printf("read %d bytes\n", r);
			newdata(buf, n);
			nodata_count = 0;
		} else if (n == 0) {
			printf("read no data\n");
			if (++nodata_count > 6) {
				close_port();
				die("No data from %s\n", PORT);
				nodata_count = 0;
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
}

void close_port(void)
{
	if (portfd >= 0) {
		close(portfd);
		portfd = -1;
	}
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





