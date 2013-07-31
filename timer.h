#ifndef TIMER_H
#define TIMER_H

#include <stdlib.h>

timer_t create_timer(int id, int frequency, void (*timer_handler));
void reset_timer(timer_t tid, int frequency);
void delete_timer(timer_t tid);

#endif