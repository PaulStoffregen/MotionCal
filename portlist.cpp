/*  Serial port object for use with wxWidgets
 *  Copyright 2010, Paul Stoffregen (paul@pjrc.com)
 */

#include "gui.h"

#if defined(LINUX)
  #include <sys/types.h>
  #include <fcntl.h>
  #include <errno.h>
  #include <sys/select.h>
  #include <termios.h>
  #include <unistd.h>
  #include <dirent.h>
  #include <sys/stat.h>
  #include <sys/ioctl.h>
  #include <linux/serial.h>
#elif defined(MACOSX)
  #include <stdio.h>
  #include <string.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <sys/ioctl.h>
  #include <errno.h>
  #include <paths.h>
  #include <termios.h>
  #include <sysexits.h>
  #include <sys/param.h>
  #include <sys/select.h>
  #include <sys/time.h>
  #include <time.h>
  #include <CoreFoundation/CoreFoundation.h>
  #include <IOKit/IOKitLib.h>
  #include <IOKit/serial/IOSerialKeys.h>
  #include <IOKit/IOBSD.h>
#elif defined(WINDOWS)
  #include <windows.h>
  #define win32_err(s) FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, \
			GetLastError(), 0, (s), sizeof(s), NULL)
  #define QUERYDOSDEVICE_BUFFER_SIZE 262144
#else
  #error "This platform is unsupported, sorry"
#endif



#if defined(LINUX)
// All linux serial port device names.  Hopefully all of them anyway.  This
// is a long list, but each entry takes only a few bytes and a quick strcmp()
static const char *devnames[] = {
"S",	// "normal" Serial Ports - MANY drivers using this
"USB",	// USB to serial converters
"ACM",	// USB serial modem, CDC class, Abstract Control Model
"MI",	// MOXA Smartio/Industio family multiport serial... nice card, I have one :-)
"MX",	// MOXA Intellio family multiport serial
"C",	// Cyclades async multiport, no longer available, but I have an old ISA one! :-)
"D",	// Digiboard (still in 2.6 but no longer supported), new Moschip MCS9901
"P",	// Hayes ESP serial cards (obsolete)
"M",	// PAM Software's multimodem & Multitech ISI-Cards
"E",	// Stallion intelligent multiport (no longer made)
"L",	// RISCom/8 multiport serial
"W",	// specialix IO8+ multiport serial
"X",	// Specialix SX series cards, also SI & XIO series
"SR",	// Specialix RIO serial card 257+
"n",	// Digi International Neo (yes lowercase 'n', drivers/serial/jsm/jsm_driver.c)
"FB",	// serial port on the 21285 StrongArm-110 core logic chip
"AM",	// ARM AMBA-type serial ports (no DTR/RTS)
"AMA",	// ARM AMBA-type serial ports (no DTR/RTS)
"AT",	// Atmel AT91 / AT32 Serial ports
"BF",	// Blackfin 5xx serial ports (Analog Devices embedded DSP chips)
"CL",	// CLPS711x serial ports (ARM processor)
"A",	// ICOM Serial
"SMX",	// Motorola IMX serial ports
"SOIC",	// ioc3 serial
"IOC",	// ioc4 serial
"PSC",	// Freescale MPC52xx PSCs configured as UARTs
"MM",	// MPSC (UART mode) on Marvell GT64240, GT64260, MV64340...
"B",	// Mux console found in some PA-RISC servers
"NX",	// NetX serial port
"PZ",	// PowerMac Z85c30 based ESCC cell found in the "macio" ASIC
"SAC",	// Samsung S3C24XX onboard UARTs
"SA",	// SA11x0 serial ports
"AM",	// KS8695 serial ports & Sharp LH7A40X embedded serial ports
"TX",	// TX3927/TX4927/TX4925/TX4938 internal SIO controller
"SC",	// Hitachi SuperH on-chip serial module
"SG",	// C-Brick Serial Port (and console) SGI Altix machines
"HV",	// SUN4V hypervisor console
"UL",	// Xilinx uartlite serial controller
"VR",	// NEC VR4100 series Serial Interface Unit
"CPM",	// CPM (SCC/SMC) serial ports; core driver
"Y",	// Amiga A2232 board
"SL",	// Microgate SyncLink ISA and PCI high speed multiprotocol serial
"SLG",	// Microgate SyncLink GT (might be sync HDLC only?)
"SLM",	// Microgate SyncLink Multiport high speed multiprotocol serial
"CH",	// Chase Research AT/PCI-Fast serial card
"F",	// Computone IntelliPort serial card
"H",	// Chase serial card
"I",	// virtual modems
"R",	// Comtrol RocketPort
"SI",	// SmartIO serial card
"T",	// Technology Concepts serial card
"V"	// Comtrol VS-1000 serial controller
};
#define NUM_DEVNAMES (sizeof(devnames) / sizeof(const char *))
#endif



#if defined(MACOSX)
static void macos_ports(io_iterator_t  * PortIterator, wxArrayString& list)
{
	io_object_t modemService;
	CFTypeRef nameCFstring;
	char s[MAXPATHLEN];

	while ((modemService = IOIteratorNext(*PortIterator))) {
		nameCFstring = IORegistryEntryCreateCFProperty(modemService, 
		   CFSTR(kIOCalloutDeviceKey), kCFAllocatorDefault, 0);
		if (nameCFstring) {
			if (CFStringGetCString((const __CFString *)nameCFstring,
			   s, sizeof(s), kCFStringEncodingASCII)) {
				list.Add(s);
			}
			CFRelease(nameCFstring);
		}
		IOObjectRelease(modemService);
	}
}
#endif


// Return a list of all serial ports
wxArrayString serial_port_list()
{
	wxArrayString list;
#if defined(LINUX)
	// This is ugly guessing, but Linux doesn't seem to provide anything else.
	// If there really is an API to discover serial devices on Linux, please
	// email paul@pjrc.com with the info.  Please?
	// The really BAD aspect is all ports get DTR raised briefly, because linux
	// has no way to open the port without raising DTR, and there isn't any way
	// to tell if the device file really represents hardware without opening it.
	// maybe sysfs or udev provides a useful API??
	DIR *dir;
	struct dirent *f;
	struct stat st;
	unsigned int i, len[NUM_DEVNAMES];
	char s[512];
	int fd, bits;
	termios mytios;

	dir = opendir("/dev/");
	if (dir == NULL) return list;
	for (i=0; i<NUM_DEVNAMES; i++) len[i] = strlen(devnames[i]);
	// Read all the filenames from the /dev directory...
	while ((f = readdir(dir)) != NULL) {
		// ignore everything that doesn't begin with "tty"
		if (strncmp(f->d_name, "tty", 3)) continue;
		// ignore anything that's not a known serial device name
		for (i=0; i<NUM_DEVNAMES; i++) {
			if (!strncmp(f->d_name + 3, devnames[i], len[i])) break;
		}
		if (i >= NUM_DEVNAMES) continue;
		snprintf(s, sizeof(s), "/dev/%s", f->d_name);
		// check if it's a character type device (almost certainly is)
		if (stat(s, &st) != 0 || !(st.st_mode & S_IFCHR)) continue;
		// now see if we can open the file - if the device file is
		// populating /dev but doesn't actually represent a loaded
		// driver, this is where we will detect it.
		fd = open(s, O_RDONLY | O_NOCTTY | O_NONBLOCK);
		if (fd < 0) {
			// if permission denied, give benefit of the doubt
			// (otherwise the port will be invisible to the user
			// and we won't have a to alert them to the permssion
			// problem)
			if (errno == EACCES) list.Add(s);
			// any other error, assume it's not a real device
			continue;
		}
		// does it respond to termios requests? (probably will since
		// the name began with tty).  Some devices where a single
		// driver exports multiple names will open but this is where
		// we can really tell if they work with real hardare.
		if (tcgetattr(fd, &mytios) != 0) {
			close(fd);
			continue;
		}
		// does it respond to reading the control signals?  If it's
		// some sort of non-serial terminal (eg, pseudo terminals)
		// this is where we will detect it's not really a serial port
		if (ioctl(fd, TIOCMGET, &bits) < 0) {
			close(fd);
			continue;
		}
		// it passed all the tests, it's a serial port, or some sort
		// of "terminal" that looks exactly like a real serial port!
		close(fd);
		// unfortunately, Linux always raises DTR when open is called.
		// not nice!  Every serial port is going to get DTR raised
		// and then lowered.  I wish there were a way to prevent this,
		// but it seems impossible.
		list.Add(s);
	}
	closedir(dir);
#elif defined(MACOSX)
	// adapted from SerialPortSample.c, by Apple
	// http://developer.apple.com/samplecode/SerialPortSample/listing2.html
	// and also testserial.c, by Keyspan
	// http://www.keyspan.com/downloads-files/developer/macosx/KesypanTestSerial.c
	// www.rxtx.org, src/SerialImp.c seems to be based on Keyspan's testserial.c
	// neither keyspan nor rxtx properly release memory allocated.
	// more documentation at:
	// http://developer.apple.com/documentation/DeviceDrivers/Conceptual/WorkingWSerial/WWSerial_SerialDevs/chapter_2_section_6.html
	mach_port_t masterPort;
	CFMutableDictionaryRef classesToMatch;
	io_iterator_t serialPortIterator;
	if (IOMasterPort(NULL, &masterPort) != KERN_SUCCESS) return list;
	// a usb-serial adaptor is usually considered a "modem",
	// especially when it implements the CDC class spec
	classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue);
	if (!classesToMatch) return list;
	CFDictionarySetValue(classesToMatch, CFSTR(kIOSerialBSDTypeKey),
	   CFSTR(kIOSerialBSDModemType));
	if (IOServiceGetMatchingServices(masterPort, classesToMatch,
	   &serialPortIterator) != KERN_SUCCESS) return list;
	macos_ports(&serialPortIterator, list);
	IOObjectRelease(serialPortIterator);
	// but it might be considered a "rs232 port", so repeat this
	// search for rs232 ports
	classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue);
	if (!classesToMatch) return list;
	CFDictionarySetValue(classesToMatch, CFSTR(kIOSerialBSDTypeKey),
	   CFSTR(kIOSerialBSDRS232Type));
	if (IOServiceGetMatchingServices(masterPort, classesToMatch,
	   &serialPortIterator) != KERN_SUCCESS) return list;
	macos_ports(&serialPortIterator, list);
	IOObjectRelease(serialPortIterator);
#elif defined(WINDOWS)
	// http://msdn.microsoft.com/en-us/library/aa365461(VS.85).aspx
	// page with 7 ways - not all of them work!
	// http://www.naughter.com/enumser.html
	// may be possible to just query the windows registary
	// http://it.gps678.com/2/ca9c8631868fdd65.html
	// search in HKEY_LOCAL_MACHINE\HARDWARE\DEVICEMAP\SERIALCOMM
	// Vista has some special new way, vista-only
	// http://msdn2.microsoft.com/en-us/library/aa814070(VS.85).aspx
	char *buffer, *p;
	//DWORD size = QUERYDOSDEVICE_BUFFER_SIZE;
	DWORD ret;

	buffer = (char *)malloc(QUERYDOSDEVICE_BUFFER_SIZE);
	if (buffer == NULL) return list;
	memset(buffer, 0, QUERYDOSDEVICE_BUFFER_SIZE);
	ret = QueryDosDeviceA(NULL, buffer, QUERYDOSDEVICE_BUFFER_SIZE);
	if (ret) {
		printf("Detect Serial using QueryDosDeviceA: ");
		for (p = buffer; *p; p += strlen(p) + 1) {
			printf(":  %s", p);
			if (strncmp(p, "COM", 3)) continue;
			list.Add(wxString(p) + ":");
		}
	} else {
		char buf[1024];
		win32_err(buf);
		printf("QueryDosDeviceA failed, error \"%s\"\n", buf);
		printf("Detect Serial using brute force GetDefaultCommConfig probing: ");
		for (int i=1; i<=32; i++) {
			printf("try  %s", buf);
			COMMCONFIG cfg;
			DWORD len;
			snprintf(buf, sizeof(buf), "COM%d", i);
			if (GetDefaultCommConfig(buf, &cfg, &len)) {
				wxString name;
				name.Printf("COM%d:", i);
				list.Add(name);
				printf(":  %s", buf);
			}
		}
	}
	free(buffer);
#endif
	list.Sort();
	return list;
}








