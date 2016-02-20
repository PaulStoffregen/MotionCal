OS = LINUX
#OS = MACOSX
#OS = WINDOWS

ifeq ($(OS), LINUX)
CC = gcc
CXX = g++
CFLAGS = -O2 -Wall -D$(OS)
CXXFLAGS = $(CFLAGS) `$(WXCONFIG) --cppflags`
LDFLAGS =
WXCONFIG = ~/wxwidgets/3.0.2.gtk2-opengl/bin/wx-config
CLILIBS = -lglut -lGLU -lGL

else ifeq ($(OS), MACOSX)
#TODO: Macintosh build...

else ifeq ($(OS), WINDOWS)
CC = i686-w64-mingw32-gcc
CXX = i686-w64-mingw32-g++
CFLAGS = -O2 -Wall -D$(OS)
CXXFLAGS = $(CFLAGS) `$(WXCONFIG) --cppflags`
LDFLAGS = -static -static-libgcc
WXCONFIG = ~/wxwidgets/3.0.2.mingw-opengl/bin/wx-config
CLILIBS = -lglut32 -lglu32 -lopengl32

endif


all: gui imuread

gui: gui.o visualize.o serialdata.o
	$(CXX) -s $(CFLAGS) $(LDFLAGS) -o $@ $^ `$(WXCONFIG) --libs all,opengl`
ifeq ($(OS), WINDOWS)
	cp gui gui.exe
endif


imuread: imuread.o visualize.o serialdata.o
	$(CC) -s $(CFLAGS) $(LDFLAGS) -o $@ $^ $(CLILIBS)

clean:
	rm -f gui imuread *.o *.exe
