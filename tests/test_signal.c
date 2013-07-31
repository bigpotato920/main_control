#include <unistd.h>
#include <stdio.h>
#include <pthread.h>

void *func(void *arg)
{
	while (1) {
		printf("child thread\n");
		sleep(5);
	}

}


int main(int argc, char const *argv[])
{
	pthread_t pid;

	pthread_create(&pid, NULL, func, NULL);
	while (1) {
		printf("main\n");
		sleep(10);
	}

	printf("exit\n");
	return 0;
}