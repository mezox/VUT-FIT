# Makefile k DU2 z predmetu IJC
# Tomas Kubovcik, xkubov02
# Datum:   22/04/2012
 
CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -pedantic #-Werror

all: readerWriter

readerWriter: readerWriter.o
	$(CC) $(CFLAGS) -o readerWriter readerWriter.o -lrt

readerWriter.o: readerWriter.c
	$(CC) $(CFLAGS) -c readerWriter.c

clean:
	rm -f *.o
