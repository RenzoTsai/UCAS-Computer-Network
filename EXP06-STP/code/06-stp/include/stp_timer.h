#ifndef __STP_TIMER_H__
#define __STP_TIMER_H__

#include "types.h"
#include "list.h"

#include <sys/time.h>

typedef void (*timeout_handler)(void *arg);

typedef struct {
	struct list_head list;
	bool active;
	long long int time;		// time when the timer is set active
	int timeout;
	timeout_handler func;
	void *arg;
} stp_timer_t;

long long int time_tick_now();
void stp_init_timer(stp_timer_t *timer, \
		int timeout, timeout_handler func, void *arg);
void stp_start_timer(stp_timer_t *timer, long long int time);
void stp_stop_timer(stp_timer_t *timer);
void stp_timer_run_once(long long int now);

#endif
