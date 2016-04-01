OS = LINUX
#OS = MACOSX
#OS = MACOSX_CLANG
#OS = WINDOWS

ifeq ($(OS), LINUX)
ALL = MotionCal imuread
CC = gcc
CXX = g++
CFLAGS = -O2 -Wall -D$(OS)
CXXFLAGS = $(CFLAGS) `$(WXCONFIG) --cppflags`
LDFLAGS =
SFLAG = -s
WXCONFIG = ~/wxwidgets/3.0.2.gtk2-opengl/bin/wx-config
CLILIBS = -lglut -lGLU -lGL -lm
MAKEFLAGS = --jobs=12

else ifeq ($(OS), MACOSX)
ALL = MotionCal.app
CC = gcc-4.2
CXX = g++-4.2
CFLAGS = -O2 -Wall -D$(OS)
CXXFLAGS = $(CFLAGS) `$(WXCONFIG) --cppflags`
WXCONFIG = ~/wxwidgets/3.0.2.mac-opengl/bin/wx-config
SFLAG = -s
CLILIBS = -lglut -lGLU -lGL -lm
VERSION = 0.01

else ifeq ($(OS), MACOSX_CLANG)
ALL = MotionCal.app
CC = /usr/bin/clang
CXX = /usr/bin/clang++
CFLAGS = -O2 -Wall -DMACOSX
WXCONFIG = wx-config
CXXFLAGS = $(CFLAGS) `$(WXCONFIG) --cppflags`
SFLAG =
CLILIBS = -lglut -lGLU -lGL -lm
VERSION = 0.01

else ifeq ($(OS), WINDOWS)
ALL = MotionCal.exe
CC = i686-w64-mingw32-gcc
CXX = i686-w64-mingw32-g++
CFLAGS = -O2 -Wall -D$(OS)
CXXFLAGS = $(CFLAGS) `$(WXCONFIG) --cppflags`
LDFLAGS = -static -static-libgcc
SFLAG = -s
WXCONFIG = ~/wxwidgets/3.0.2.mingw-opengl/bin/wx-config
CLILIBS = -lglut32 -lglu32 -lopengl32 -lm
MAKEFLAGS = --jobs=12

endif

OBJS = visualize.o serialdata.o rawdata.o magcal.o matrix.o fusion.o quality.o mahony.o

all: $(ALL)

MotionCal: gui.o portlist.o $(OBJS)
	$(CXX) $(SFLAG) $(CFLAGS) $(LDFLAGS) -o $@ $^ `$(WXCONFIG) --libs all,opengl`

MotionCal.exe: MotionCal
	cp MotionCal $@
	-./cp_windows.sh $@

MotionCal.app: MotionCal Info.plist
	mkdir -p $@/Contents/MacOS
	mkdir -p $@/Contents/Resources/English.lproj
	sed "s/1.234/$(VERSION)/g" Info.plist > $@/Contents/Info.plist
	/bin/echo -n 'APPL????' > $@/Contents/PkgInfo
	cp $< $@/Contents/MacOS/
	#cp icon.icns $@/Contents/Resources/
	touch $@

imuread: imuread.o $(OBJS)
	$(CC) -s $(CFLAGS) $(LDFLAGS) -o $@ $^ $(CLILIBS)

clean:
	rm -f gui MotionCal imuread *.o *.exe
	rm -rf MotionCal.app .DS_Store

gui.o: gui.cpp gui.h imuread.h Makefile
portlist.o: portlist.cpp gui.h Makefile
imuread.o: imuread.c imuread.h Makefile
visualize.o: visualize.c imuread.h Makefile
serialdata.o: serialdata.c imuread.h Makefile
rawdata.o: rawdata.c imuread.h Makefile
magcal.o: magcal.c imuread.h Makefile
matrix.o: matrix.c imuread.h Makefile
fusion.o: fusion.c imuread.h Makefile
quality.o: quality.c imuread.h Makefile
mahony.o: mahony.c imuread.h Makefile

