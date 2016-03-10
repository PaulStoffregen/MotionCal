OS = LINUX
#OS = MACOSX
#OS = WINDOWS

ifeq ($(OS), LINUX)
ALL = gui imuread
CC = gcc
CXX = g++
CFLAGS = -O2 -Wall -D$(OS)
CXXFLAGS = $(CFLAGS) `$(WXCONFIG) --cppflags`
LDFLAGS =
WXCONFIG = ~/wxwidgets/3.0.2.gtk2-opengl/bin/wx-config
CLILIBS = -lglut -lGLU -lGL -lm
MAKEFLAGS = --jobs=12

else ifeq ($(OS), MACOSX)
ALL = gui.app
CC = gcc-4.2
CXX = g++-4.2
CFLAGS = -O2 -Wall -D$(OS)
CXXFLAGS = $(CFLAGS) `$(WXCONFIG) --cppflags`
WXCONFIG = ~/wxwidgets/3.0.2.mac-opengl/bin/wx-config
CLILIBS = -lglut -lGLU -lGL -lm

else ifeq ($(OS), WINDOWS)
ALL = gui.exe
CC = i686-w64-mingw32-gcc
CXX = i686-w64-mingw32-g++
CFLAGS = -O2 -Wall -D$(OS)
CXXFLAGS = $(CFLAGS) `$(WXCONFIG) --cppflags`
LDFLAGS = -static -static-libgcc
WXCONFIG = ~/wxwidgets/3.0.2.mingw-opengl/bin/wx-config
CLILIBS = -lglut32 -lglu32 -lopengl32 -lm
VERSION = 0.01

endif

OBJS = visualize.o serialdata.o rawdata.o magcal.o matrix.o

all: $(ALL)

gui: gui.o $(OBJS)
	$(CXX) -s $(CFLAGS) $(LDFLAGS) -o $@ $^ `$(WXCONFIG) --libs all,opengl`

gui.exe: gui
	cp gui gui.exe

gui.app: gui Info.plist
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
	rm -f gui imuread *.o *.exe
	rm -rf gui.app .DS_Store

gui.o: gui.cpp gui.h imuread.h
imuread.o: imuread.c imuread.h
visualize.o: visualize.c imuread.h
serialdata.o: serialdata.c imuread.h
rawdata.o: rawdata.c imuread.h


