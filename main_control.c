#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#include "network.h"
#include "timer.h"

#define PROCESS_NUM 1
#define NAME_LENGHT 10
#define TIMEOUT_TIME 20
#define CONFIG_PATH "config.ini"

char child_names[PROCESS_NUM][NAME_LENGHT];
char unix_server_name[BUFSIZ];
timer_t timers[PROCESS_NUM];

void timeout(union sigval v);
void read_config(char *filename);
void start_timers();
void reset(int id);
void delete_timers();
int start_child_process(int i);
int start_child_processes();
void wait_heart_beat(int server_fd);

/**
 * timer handler
 * @param value send by the timer when the timer alarmed
 */
void timeout(union sigval v)
{
	printf("hello:id = %d\n", v.sival_int);
	start_child_process(v.sival_int);
}

/**
 * read the configuration from the file
 * @param filename configuration filename
 */
void read_config(char *filename)
{
	FILE *fp = fopen(filename, "r");
	char key[BUFSIZ];
	char val[BUFSIZ];
	int i = 0;

	while ((fscanf(fp, "%s %s", key, val)) == 2) {
		if (strcmp(key, "PROCESS_NAME") == 0) {
			strcpy(child_names[i], val);
			i++;
		} else if (strcmp(key, "UNIX_SERVER_PATH") == 0) {
			strcpy(unix_server_name, val);
		}
	}

	fclose(fp);

	for (i = 0; i < PROCESS_NUM; i++)
		printf("name = %s\n", child_names[i]);
	
}

/**
 * trigger the timer for each process
 */
void start_timers()
{
	int i;

	for (i = 0; i < PROCESS_NUM; i++) {
		timers[i] = create_timer(i, TIMEOUT_TIME, timeout);
	}
}


/**
 * reset the timer
 * @param id timer id
 */
void reset(int id)
{

	reset_timer(timers[id], TIMEOUT_TIME);
}

/**
 * delete all the timers
 */
void delete_timers()
{
	int i;
	for (i = 0; i < PROCESS_NUM; i++) {
		delete_timer(timers[i]);
	}
}
/**
 * start a child process
 * @param  index the index of the process
 * @return       0 on success or -1 on failure
 */
int start_child_process(int index)
{
	int rv;
	rv = system(child_names[index]);
	if (rv == -1) {
		fprintf(stderr, "failed to create process:%s\n", child_names[index]);
		return -1;
	}
	return 0;
}

/**
 * start the child processes
 * @return	0 on success or -1 on failure
 */
int start_child_processes()
{
	int  i;
	int rv;

	for (i = 0; i < PROCESS_NUM; i++) {
		rv = start_child_process(i);
		if (rv == -1)
			return -1;
	}

	return 0;
}


/**
 * waiting the hearbeat send by the child process
 * @param server_fd [description]
 */
void wait_heart_beat(int server_fd) 
{

 	int client_fd;
    int nread;
    int tmp_fd;
    int flag;
    int rv;

    fd_set readfds;
    fd_set testfds;

    FD_ZERO(&readfds);
    FD_SET(server_fd, &readfds);

    while (1) {

        testfds = readfds;
        rv = select(FD_SETSIZE, &testfds, (fd_set *)NULL, (fd_set *)NULL, (struct timeval *)NULL);
        switch (rv)
        {
    
        case -1:
            perror("select");
            break;
        default:

        	for (tmp_fd = 0; tmp_fd < FD_SETSIZE; tmp_fd++) {
        		if (FD_ISSET(tmp_fd, &testfds)) {

        			if (tmp_fd == server_fd) {
            			client_fd = server_accept(server_fd);
            			printf("a client connected\n");
            			FD_SET(client_fd, &readfds);
            	
            		} else {
            			nread = read(tmp_fd, &flag, sizeof(flag));
            			if (nread > 0) {
            				printf("flag = %d\n", flag);
            				reset(flag);
            			} else if (nread == 0) {
            				printf("a client disconnected\n");
            				FD_CLR(tmp_fd, &readfds);
            				close(tmp_fd);
            			} else {
            				FD_CLR(tmp_fd, &readfds);
            				close(tmp_fd);
            			}
            		}
        		}
	
        	}
        	break;
        }
        	
    }
}


int main(int argc, char const *argv[])
{
	int rv;
	int server_fd;

	read_config(CONFIG_PATH);

	server_fd = create_unix_server(unix_server_name);
	if (server_fd < 0)
		return 1;

	rv = start_child_processes();
	if (rv < 0)
		return 1;

	start_timers();
	wait_heart_beat(server_fd);
	
	delete_timers();
	close(server_fd);

	return 0;
}