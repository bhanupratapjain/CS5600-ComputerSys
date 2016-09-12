#include <stdio.h>
#include <signal.h>

void sighandler(int signum)
{
   printf("Caught signal %d\n", signum);
 
}
__attribute__ ((constructor))
void myconstructor() {
  signal(SIGUSR2, sighandler);
}
