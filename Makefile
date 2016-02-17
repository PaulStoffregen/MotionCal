CC = gcc
CFLAGS = -O2 -Wall
CPPFLAGS = `$(WXCONFIG) --cppflags`
WXCONFIG = ~/wxwidgets/3.0.2.gtk2-opengl/bin/wx-config


all: gui imuread

gui: gui.o visualize.o serialdata.o
	g++ $(CFLAGS) -o $@ $^ `$(WXCONFIG) --libs` -lglut -lGLU -lGL

imuread: imuread.o visualize.o serialdata.o
	$(CC) $(CFLAGS) -o $@ $^ -lglut -lGLU -lGL

clean:
	rm -f gui imuread *.o
