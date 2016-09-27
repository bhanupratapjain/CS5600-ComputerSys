CFLAGS=-g -O0 -fpic -Wall
#-Werror

all:	clean-ckpt count myrestart dockpt

default: clean-ckpt count myrestart dockpt

check:	clean-ckpt count myrestart libckpt.so
	(sleep 3 && kill -12 `pgrep -n count` && sleep 2 && pkill -9 count) & 
	LD_PRELOAD=`pwd`/libckpt.so ./count
	(sleep 2 &&  pkill -9 restart) &
	make res

build: count myrestart libckpt.so

util.o: util.c util.h
	gcc $(CFLAGS) -c util.c -o util.o

clean-ckpt:
	rm -rf myckpt

ckpt.o: ckpt.c util.h
	gcc $(CFLAGS) -c ckpt.c -o ckpt.o

count:  count.c
	gcc $(CFLAGS) count.c -o count

libckpt.so: ckpt.o util.o
	gcc -shared -o libckpt.so ckpt.o util.o

dockpt: libckpt.so
	LD_PRELOAD=`pwd`/libckpt.so ./count

myrestart.o: myrestart.c util.h
	gcc $(CFLAGS) -c -o myrestart.o myrestart.c

myrestart: myrestart.o util.o
	gcc -g -static -Wl,-Ttext-segment=5000000 -Wl,-Tdata=5100000 -Wl,-Tbss=5200000 -o myrestart myrestart.o util.o

res: myrestart
	./myrestart myckpt

clean:
	rm -f *.o *.so myrestart count myckpt
