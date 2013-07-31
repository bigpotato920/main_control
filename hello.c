#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include "timer.h"
#define SERVER_PATH "/var/tmp/main_control"

void *thread_func(void *arg)
{
	while (1) {
		printf("thread func...\n");
		sleep(2);
	}
}

int init_heartbeat_service()
{
	int client_fd;
	int addr_len;

	struct sockaddr_un server_addr;

	if ((client_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror("create socket");
		return -1;
	}

	memset(&server_addr, 0, sizeof(struct sockaddr_un));
	server_addr.sun_family = AF_UNIX;
	strcpy(server_addr.sun_path, SERVER_PATH);
	addr_len = offsetof(struct sockaddr_un, sun_path) + strlen(server_addr.sun_path);

	if (connect(client_fd, (struct sockaddr*)&server_addr, addr_len) < 0) {
		perror("socket connect");
		return -1;
	}

	return client_fd;
}

void send_heartbeat(union sigval v)
{
	int flag = 0;
	int nwrite;
	int client_fd = v.sival_int;

	nwrite = write(client_fd, &flag, sizeof(flag));
	printf("hello nwrite = %d\n", nwrite);

}

int main(int argc, char const *argv[])
{

	pthread_t pid;

	pthread_create(&pid, NULL, thread_func, NULL);
	timer_t tid;
	int client_fd = init_heartbeat_service();

	tid = create_timer(client_fd, 10, send_heartbeat);

	while (1) {
		printf("hello main..\n");
		sleep(15);
	}
	close(client_fd);
	pthread_join(pid, NULL);
	delete_timer(tid);
	return 0;
}