#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>

#include "timer.h"

void handler(union sigval v)
{
	printf("hello:id = %d\n", v.sival_int);
}


int main(int argc, char const *argv[])
{
	
	timer_t t1;
	timer_t t2;

	t1 = create_timer(1, 1, handler);
	t2 = create_timer(2, 10, handler);

	//while (1) {
		printf("main ..\n");
		sleep(8);
		reset_timer(t2, 10);
		sleep(20);
	//}

	return 0;
}