# Create Shared Library4
gcc -c -Wall -Werror -fpic libfoo.c -o libfoo
gcc -shared -o libfoo.so libfoo
export LD_PRELOAD=/home/bhanupratapjain/CS5600-ComputerSys/hw01/libckpt.so cout
