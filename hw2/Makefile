#Sample Makefile for Malloc
CC=gcc
#CFLAGS=-g -O0 -fPIC -pthread
CFLAGS=-g -O0 -fPIC -pthread -fpack-struct

all:	check

clean:
	rm -rf *.so *.o

libmalloc.so: malloc.o arena.o bin.o block.o calloc.o realloc.o free.o stats.o
	$(CC) $(CFLAGS) -shared -Wl,--unresolved-symbols=ignore-all -o libmalloc.so arena.o bin.o block.o malloc.o calloc.o realloc.o free.o stats.o

test1: test1.o
	$(CC) $(CFLAGS) $< -o $@

t-test1: t-test1.o
	$(CC) $(CFLAGS) $< -o $@

# For every XYZ.c file, generate XYZ.o.
%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@

check: libmalloc.so t-test1 test1
#   LD_PRELOAD=`pwd`/libmalloc.so ./test1
	LD_PRELOAD=`pwd`/libmalloc.so ./t-test1

dist:
	dir=`basename $$PWD`; cd ..; tar cvf $$dir.tar ./$$dir; gzip $$dir.tar


# set exec-wrapper env 'LD_PRELOAD=/home/bhanupratapjain/CS5600-ComputerSys/hw2/libmalloc.so'