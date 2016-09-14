all:make
	echo Make Successful

default: make
	echo Make Successful

make: make-main-binary load-so

make-main-binary:  
	gcc count.c -o count

make-so:make-so-binary
	gcc -shared -o libckpt.so libckpt

load-so:make-so
	export LD_PRELOAD=/home/bhanupratapjain/CS5600-ComputerSys/hw01/libckpt.so count

make-so-binary:
	gcc -c -Wall -Werror -fpic libckpt.c -o libckpt