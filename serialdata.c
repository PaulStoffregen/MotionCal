#include "imuread.h"


void print_data(const char *name, const unsigned char *data, int len)
{
	int i;

	printf("%s (%2d bytes):", name, len);
	for (i=0; i < len; i++) {
		printf(" %02X", data[i]);
	}
	printf("\n");
}

static int packet_primary_data(const unsigned char *data)
{
	//current_position.x = (float)((int16_t)((data[13] << 8) | data[12])) / 10.0f;
	//current_position.y = (float)((int16_t)((data[15] << 8) | data[14])) / 10.0f;
	//current_position.z = (float)((int16_t)((data[17] << 8) | data[16])) / 10.0f;
	current_orientation.q0 = (float)((int16_t)((data[25] << 8) | data[24])) / 30000.0f;
	current_orientation.q1 = (float)((int16_t)((data[27] << 8) | data[26])) / 30000.0f;
	current_orientation.q2 = (float)((int16_t)((data[29] << 8) | data[28])) / 30000.0f;
	current_orientation.q3 = (float)((int16_t)((data[31] << 8) | data[30])) / 30000.0f;
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
	return 1;
}

static int packet_magnetic_cal(const unsigned char *data)
{
	int16_t id, x, y, z;
	int n;

	id = (data[7] << 8) | data[6];
	x = (data[9] << 8) | data[8];
	y = (data[11] << 8) | data[10];
	z = (data[13] << 8) | data[12];

	if (id == 1) {
		magcal.fV[0] = (float)x * 0.1f;
		magcal.fV[1] = (float)y * 0.1f;
		magcal.fV[2] = (float)z * 0.1f;
		return 1;
	} else if (id == 2) {
		magcal.finvW[0][0] = (float)x * 0.001f;
		magcal.finvW[1][1] = (float)y * 0.001f;
		magcal.finvW[2][2] = (float)z * 0.001f;
		return 1;
	} else if (id == 3) {
		magcal.finvW[0][1] = (float)x / 1000.0f;
		magcal.finvW[1][0] = (float)x / 1000.0f; // TODO: check this assignment
		magcal.finvW[0][2] = (float)y / 1000.0f;
		magcal.finvW[1][2] = (float)y / 1000.0f; // TODO: check this assignment
		magcal.finvW[1][2] = (float)z / 1000.0f;
		magcal.finvW[2][1] = (float)z / 1000.0f; // TODO: check this assignment
		return 1;
	} else if (id >= 10 && id < MAGBUFFSIZE+10) {
		n = id - 10;
		if (magcal.valid[n] == 0 || x != magcal.iBpFast[0][n]
		  || y != magcal.iBpFast[1][n] || z != magcal.iBpFast[2][n]) {
			magcal.iBpFast[0][n] = x;
			magcal.iBpFast[1][n] = y;
			magcal.iBpFast[2][n] = z;
			magcal.valid[n] = 1;
			printf("mag cal, n=%3d: %5d %5d %5d\n", n, x, y, z);
		}
		return 1;
	}
	return 0;
}

static int packet(const unsigned char *data, int len)
{
	if (len <= 0) return 0;
	//print_data("packet", data, len);

	if (data[0] == 1 && len == 34) {
		return packet_primary_data(data);
	} else if (data[0] == 6 && len == 14) {
		return packet_magnetic_cal(data);
	}
	return 0;
}

static int packet_encoded(const unsigned char *data, int len)
{
	const unsigned char *p;
	unsigned char buf[256];
	int buflen=0, copylen;

	//printf("packet_encoded, len = %d\n", len);
	p = memchr(data, 0x7D, len);
	if (p == NULL) {
		return packet(data, len);
	} else {
		//printf("** decoding necessary\n");
		while (1) {
			copylen = p - data;
			if (copylen > 0) {
				//printf("  copylen = %d\n", copylen);
				if (buflen + copylen > sizeof(buf)) return 0;
				memcpy(buf+buflen, data, copylen);
				buflen += copylen;
				data += copylen;
				len -= copylen;
			}
			if (buflen + 1 > sizeof(buf)) return 0;
			buf[buflen++] = (p[1] == 0x5E) ? 0x7E : 0x7D;
			data += 2;
			len -= 2;
			if (len <= 0) break;
			p = memchr(data, 0x7D, len);
			if (p == NULL) {
				if (buflen + len > sizeof(buf)) return 0;
				memcpy(buf+buflen, data, len);
				buflen += len;
				break;
			}
		}
		//printf("** decoded to %d\n", buflen);
		return packet(buf, buflen);
	}
}

static int packet_parse(const unsigned char *data, int len)
{
	static unsigned char packetbuf[256];
	static unsigned int packetlen=0;
	const unsigned char *p;
	int copylen;
	int ret=0;

	//print_data("packet_parse", data, len);
	while (len > 0) {
		p = memchr(data, 0x7E, len);
		if (p == NULL) {
			if (packetlen + len > sizeof(packetbuf)) {
				packetlen = 0;
				return 0;  // would overflow buffer
			}
			memcpy(packetbuf+packetlen, data, len);
			packetlen += len;
			len = 0;
		} else if (p > data) {
			copylen = p - data;
			if (packetlen + copylen > sizeof(packetbuf)) {
				packetlen = 0;
				return 0;  // would overflow buffer
			}
			memcpy(packetbuf+packetlen, data, copylen);
			packet_encoded(packetbuf, packetlen+copylen);
			packetlen = 0;
			data += copylen + 1;
			len -= copylen + 1;
		} else {
			if (packetlen > 0) {
				if (packet_encoded(packetbuf, packetlen)) ret = 1;
				packetlen = 0;
			}
			data++;
			len--;
		}
	}
	return ret;
}

static int ascii_parse(const unsigned char *data, int len)
{
	static int ascii_num=0, ascii_neg=0, ascii_count=0;
	static int16_t ascii_raw_data[9];
	static unsigned int ascii_raw_data_count=0;
	const char *p, *end;
	int ret=0;

	//print_data("ascii_parse", data, len);
	end = (const char *)(data + len);
	for (p = (const char *)data ; p < end; p++) {
		if (*p == '-') {
			//printf("ascii_parse negative\n");
			if (ascii_count > 0) goto fail;
			ascii_neg = 1;
		} else if (isdigit(*p)) {
			//printf("ascii_parse digit\n");
			ascii_num = ascii_num * 10 + *p - '0';
			ascii_count++;
		} else if (*p == ',') {
			//printf("ascii_parse comma, %d\n", ascii_num);
			if (ascii_neg) ascii_num = -ascii_num;
			if (ascii_num < -32768 && ascii_num > 32767) goto fail;
			if (ascii_raw_data_count >= 8) goto fail;
			ascii_raw_data[ascii_raw_data_count++] = ascii_num;
			ascii_num = 0;
			ascii_neg = 0;
			ascii_count = 0;
		} else if (*p == 13) {
			//printf("ascii_parse newline\n");
			if (ascii_neg) ascii_num = -ascii_num;
			if (ascii_num < -32768 && ascii_num > 32767) goto fail;
			if (ascii_raw_data_count != 8) goto fail;
			ascii_raw_data[ascii_raw_data_count] = ascii_num;
			raw_data(ascii_raw_data);
			ret = 1;
			ascii_raw_data_count = 0;
			ascii_num = 0;
			ascii_neg = 0;
			ascii_count = 0;
		} else if (*p == 10) {
		} else {
			goto fail;
		}
	}
	return ret;
fail:
	ascii_raw_data_count = 0;
	ascii_num = 0;
	ascii_neg = 0;
	ascii_count = 0;
	return 0;
}


static void newdata(const unsigned char *data, int len)
{
	packet_parse(data, len);
	ascii_parse(data, len);
	// TODO: learn which one and skip the other
}


#if defined(LINUX) || defined(MACOSX)

static int portfd=-1;

int open_port(const char *name)
{
	struct termios termsettings;
	int r;

	portfd = open(name, O_RDWR | O_NONBLOCK);
	if (portfd < 0) return 0;
	r = tcgetattr(portfd, &termsettings);
	if (r < 0) {
		close_port();
		return 0;
	}
	cfmakeraw(&termsettings);
	cfsetspeed(&termsettings, B115200);
	r = tcsetattr(portfd, TCSANOW, &termsettings);
	if (r < 0) {
		close_port();
		return 0;
	}
	return 1;
}

int read_serial_data(void)
{
	unsigned char buf[256];
	static int nodata_count=0;
	int n;

	if (portfd < 0) return -1;
	while (1) {
		n = read(portfd, buf, sizeof(buf));
		if (n > 0 && n <= sizeof(buf)) {
			newdata(buf, n);
			nodata_count = 0;
			//return n;
		} else if (n == 0) {
			if (++nodata_count > 6) {
				close_port();
				nodata_count = 0;
				close_port();
				return -1;
			}
			return 0;
		} else {
			n = errno;
			if (n == EAGAIN) {
				return 0;
			} else if (n == EINTR) {
			} else {
				close_port();
				return -1;
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

#elif defined(WINDOWS)

static HANDLE port_handle;

int open_port(const char *name)
{
	COMMCONFIG port_cfg;
	COMMTIMEOUTS timeouts;
	DWORD len;
	char buf[64];
	int n;

	if (strncmp(name, "COM", 3) == 0 && sscanf(name + 3, "%d", &n) == 1) {
		snprintf(buf, sizeof(buf), "\\\\.\\COM%d", n);
		name = buf;
	}
	port_handle = CreateFile(name, GENERIC_READ | GENERIC_WRITE,
		0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (port_handle == INVALID_HANDLE_VALUE) {
		return 0;
	}
	len = sizeof(COMMCONFIG);
	if (!GetCommConfig(port_handle, &port_cfg, &len)) {
		CloseHandle(port_handle);
		return 0;
	}
	port_cfg.dcb.BaudRate = 115200;
	port_cfg.dcb.fBinary = TRUE;
	port_cfg.dcb.fParity = FALSE;
	port_cfg.dcb.fOutxCtsFlow = FALSE;
	port_cfg.dcb.fOutxDsrFlow = FALSE;
	port_cfg.dcb.fDtrControl = DTR_CONTROL_DISABLE;
	port_cfg.dcb.fDsrSensitivity = FALSE;
	port_cfg.dcb.fTXContinueOnXoff = TRUE;  // ???
	port_cfg.dcb.fOutX = FALSE;
	port_cfg.dcb.fInX = FALSE;
	port_cfg.dcb.fErrorChar = FALSE;
	port_cfg.dcb.fNull = FALSE;
	port_cfg.dcb.fRtsControl = RTS_CONTROL_DISABLE;
	port_cfg.dcb.fAbortOnError = FALSE;
	port_cfg.dcb.ByteSize = 8;
	port_cfg.dcb.Parity = NOPARITY;
	port_cfg.dcb.StopBits = ONESTOPBIT;
	if (!SetCommConfig(port_handle, &port_cfg, sizeof(COMMCONFIG))) {
		CloseHandle(port_handle);
		return 0;
	}
	if (!EscapeCommFunction(port_handle, CLRDTR | CLRRTS)) {
		CloseHandle(port_handle);
		return 0;
	}
        timeouts.ReadIntervalTimeout            = MAXDWORD;
        timeouts.ReadTotalTimeoutMultiplier     = 0;
        timeouts.ReadTotalTimeoutConstant       = 0;
        timeouts.WriteTotalTimeoutMultiplier    = 0;
        timeouts.WriteTotalTimeoutConstant      = 0;
        if (!SetCommTimeouts(port_handle, &timeouts)) {
		CloseHandle(port_handle);
		return 0;
	}
	return 1;
}

int read_serial_data(void)
{
	COMSTAT st;
	DWORD errmask=0, num_read, num_request;
	OVERLAPPED ov;
	unsigned char buf[256];
	int r;

	while (1) {
		if (!ClearCommError(port_handle, &errmask, &st)) return -1;
		//printf("Read, %d requested, %lu buffered\n", count, st.cbInQue);
		if (st.cbInQue <= 0) return 0;
		// now do a ReadFile, now that we know how much we can read
		// a blocking (non-overlapped) read would be simple, but win32
		// is all-or-nothing on async I/O and we must have it enabled
		// because it's the only way to get a timeout for WaitCommEvent
		if (st.cbInQue < (DWORD)sizeof(buf)) {
			num_request = st.cbInQue;
		} else {
			num_request = (DWORD)sizeof(buf);
		}
		ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (ov.hEvent == NULL) {
			close_port();
			return -1;
		}
		ov.Internal = ov.InternalHigh = 0;
		ov.Offset = ov.OffsetHigh = 0;
		if (ReadFile(port_handle, buf, num_request, &num_read, &ov)) {
			// this should usually be the result, since we asked for
			// data we knew was already buffered
			//printf("Read, immediate complete, num_read=%lu\n", num_read);
			r = num_read;
		} else {
			if (GetLastError() == ERROR_IO_PENDING) {
				if (GetOverlappedResult(port_handle, &ov, &num_read, TRUE)) {
					//printf("Read, delayed, num_read=%lu\n", num_read);
					r = num_read;
				} else {
					//printf("Read, delayed error\n");
					r = -1;
				}
			} else {
				//printf("Read, error\n");
				r = -1;
			}
		}
		CloseHandle(ov.hEvent);
		if (r <= 0) break;
		newdata(buf, r);
	}
        return r;
}

void close_port(void)
{
	CloseHandle(port_handle);
	port_handle = NULL;
}


#endif
