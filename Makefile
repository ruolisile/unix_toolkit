CC=gcc
CFLAG=-std=gnu99 -Wall -pedantic

all: mytoolkit mytree mymtimes
mytoolkit: mytoolkit.c
	$(CC) $(CFLAG) -o mytoolkit mytoolkit.c
mytree: mytree.c
	$(CC) $(CFLAG) -o mytree mytree.c
mymtimes: mymtimes.c
	$(CC) $(CFLAG) -o mymtimes mymtimes.c
