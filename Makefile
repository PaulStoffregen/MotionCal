CC = gcc
CFLAGS = -O2 -Wall

imuread: imuread.o serialdata.o
	$(CC) $(CFLAGS) -o $@ $^ -lglut -lGLU -lGL

clean:
	rm -f imuread *.o
