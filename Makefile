
imuread: imuread.c
	gcc -O2 -Wall -o imuread imuread.c -lglut -lGLU -lGL

clean:
	rm -f imuread
