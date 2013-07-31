#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>

#include "timer.h"

/**
 * create a timer specified by id
 * @param  id       timer id specified by the user
 * @param  timer_handler callback function when the timer alarmed
 * @return timer id specified by the system call
 */
timer_t create_timer(int id, int frequency, void (*timer_handler)) 
{

	timer_t tid;
	struct sigevent sev;
	struct itimerspec timeout;
	int rv;


	memset(&sev, 0, sizeof(sev));
	sev.sigev_notify = SIGEV_THREAD;
	sev.sigev_value.sival_int = id;
	sev.sigev_notify_function = timer_handler;


	timeout.it_interval.tv_sec = frequency;
	timeout.it_interval.tv_nsec = 0;
	timeout.it_value.tv_sec = frequency;
	timeout.it_value.tv_nsec = 0;

	rv = timer_create(CLOCK_REALTIME, &sev, &tid);
	if (rv < 0) {
		perror("failed to create timer");
		return NULL;
	}

	rv = timer_settime(tid, 0, &timeout, NULL);
	if (rv < 0) {
		perror("failed to set time");
		return NULL;
	}

	return tid;
}

/**
 * reset the timer by specific timer id
 * @param  tid       timer id
 * @param  frequency frequency of the timer
 */
void reset_timer(timer_t tid, int frequency)
{
	struct itimerspec timeout;

	timeout.it_interval.tv_sec = frequency;
	timeout.it_interval.tv_nsec = 0;
	timeout.it_value.tv_sec = frequency;
	timeout.it_value.tv_nsec = 0;

	timer_settime(tid, 0, &timeout, NULL);
}

/**
 * delete a specific timer by the timer id
 * @param tid timer id
 */
void delete_timer(timer_t tid)
{
	timer_delete(tid);
}