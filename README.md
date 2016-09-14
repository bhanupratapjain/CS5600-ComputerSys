# Create Shared Library4
gcc -c -Wall -Werror -fpic libckpt.c -o libckpt
gcc -shared -o libckpt.so libckpt
export LD_PRELOAD=/home/bhanupratapjain/CS5600-ComputerSys/hw01/libckpt.so cout
