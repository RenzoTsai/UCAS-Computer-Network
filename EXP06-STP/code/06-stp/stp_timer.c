#include "stp_timer.h"

#include "log.h"

bool timer_list_initialized = false;

struct list_head timer_list;

// one tick is 1/256 second
long long int time_tick_now()
{
	struct timeval now;
	gettimeofday(&now, NULL);
	
	return (long long int)(now.tv_sec) * 256 + now.tv_usec * 256 / 1000000;
}

void stp_init_timer(stp_timer_t *timer, \
		int timeout, timeout_handler func, void *arg) 
{
	if (!timer_list_initialized) {
		init_list_head(&timer_list);
		timer_list_initialized = true;
	}

	init_list_head(&timer->list);

	timer->active = false;
	timer->timeout = timeout;
	timer->func = func;
	timer->arg = arg;

	list_add_tail(&timer->list, &timer_list);
}

void stp_start_timer(stp_timer_t *timer, long long int time)
{
	timer->active = true;
	timer->time = time;
}

void stp_stop_timer(stp_timer_t *timer)
{
	timer->active = false;
}

bool stp_check_timer(stp_timer_t *timer, long long int now)
{
	if (timer->active) {
		if (now >= timer->time + timer->timeout) {
			timer->active = false;
			return true;
		}
	}
	return false;
}

void stp_timer_run_once(long long int now)
{
	if (!timer_list_initialized) {
		log(ERROR, "no timer in the list.");
		return ;
	}

	stp_timer_t *timer;
	list_for_each_entry(timer, &timer_list, list) {
		if (stp_check_timer(timer, now))
			timer->func(timer->arg);
	}
}
