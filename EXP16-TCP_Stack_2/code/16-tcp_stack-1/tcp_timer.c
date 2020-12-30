#include "tcp.h"
#include "tcp_timer.h"
#include "tcp_sock.h"

#include <stdio.h>
#include <unistd.h>

static struct list_head timer_list;

// scan the timer_list, find the tcp sock which stays for at 2*MSL, release it
void tcp_scan_timer_list()
{
	//fprintf(stdout, "TODO: implement %s please.\n", __FUNCTION__);
	struct tcp_timer * time_entry, *time_q;
	list_for_each_entry_safe (time_entry, time_q, &timer_list, list) {
		if (time_entry->enable == 1 && (time(NULL) * TCP_MSL - time_entry->timeout * TCP_MSL > TCP_TIMEWAIT_TIMEOUT)) {
			struct tcp_sock * tsk = timewait_to_tcp_sock(time_entry);
			list_delete_entry(&time_entry->list);
			tcp_set_state(tsk, TCP_CLOSED);
			tcp_bind_unhash(tsk);
		}
	}
}

// set the timewait timer of a tcp sock, by adding the timer into timer_list
void tcp_set_timewait_timer(struct tcp_sock *tsk)
{
	fprintf(stdout, "TODO: implement %s here.\n", __FUNCTION__);
	tsk->timewait.enable = 1;
	tsk->timewait.type = 0;
	tsk->timewait.timeout = time(NULL);
	list_add_tail(&tsk->timewait.list, &timer_list);
}

// scan the timer_list periodically by calling tcp_scan_timer_list
void *tcp_timer_thread(void *arg)
{
	init_list_head(&timer_list);
	while (1) {
		usleep(TCP_TIMER_SCAN_INTERVAL);
		tcp_scan_timer_list();
	}

	return NULL;
}
