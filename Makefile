
imuread: imuread.c
	gcc -O2 -Wall -o imuread imuread.c

clean:
	rm -f imuread
