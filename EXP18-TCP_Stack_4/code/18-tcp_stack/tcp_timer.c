#include "tcp.h"
#include "tcp_timer.h"
#include "tcp_sock.h"

#include <stdio.h>
#include <unistd.h>

static struct list_head timer_list;
static struct list_head retrans_timer_list;

#ifndef max
#	define max(x,y) ((x)>(y) ? (x) : (y))
#endif

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

// scan the restrans_timer_list periodically by calling tcp_scan_retrans_timer_list
void *tcp_retrans_timer_thread(void *arg)
{
	init_list_head(&retrans_timer_list);
	while(1){
		usleep(TCP_RETRANS_SCAN_INTERVAL);
		tcp_scan_retrans_timer_list();
	}

	return NULL;
}

// set the restrans timer of a tcp sock, by adding the timer into timer_list
void tcp_set_retrans_timer(struct tcp_sock *tsk)
{
	//printf("Set retrans timer here\n");
	if (tsk->retrans_timer.enable) {
		//printf("Already set up one retrans timer");
		return;
	}
	tsk->retrans_timer.type = 1;
	tsk->retrans_timer.timeout = TCP_RETRANS_INTERVAL_INITIAL;
	tsk->retrans_timer.retrans_time = 0;
	init_list_head(&tsk->retrans_timer.list);
	list_add_tail(&tsk->retrans_timer.list, &retrans_timer_list);
}

// update the restrans timer of a tcp sock
void tcp_update_retrans_timer(struct tcp_sock *tsk)
{
	//printf("Update retrans timer here\n");
	if (!tsk->retrans_timer.enable) {
		tcp_set_retrans_timer(tsk);
	}
	tsk->retrans_timer.type = 1;
	tsk->retrans_timer.timeout = TCP_RETRANS_INTERVAL_INITIAL;
	tsk->retrans_timer.retrans_time  = 0;
}

void tcp_unset_retrans_timer(struct tcp_sock *tsk)
{
	//printf("Delete retrans timer here\n");
	list_delete_entry(&tsk->retrans_timer.list);
}

void tcp_scan_retrans_timer_list()
{
	struct tcp_sock *tsk;
	struct tcp_timer * time_entry, *time_q;
	// if(list_empty(&retrans_timer_list)){
	// 	printf("****timer empty****\n");
	// }
	list_for_each_entry_safe(time_entry, time_q, &retrans_timer_list, list) {
		time_entry->timeout -= TCP_RETRANS_SCAN_INTERVAL;
		tsk = retranstimer_to_tcp_sock(time_entry);
		if (time_entry->timeout <= 0) {
			if(time_entry->retrans_time >= 5 && tsk->state != TCP_CLOSED){
				list_delete_entry(&time_entry->list);
				if (!tsk->parent) {
					tcp_bind_unhash(tsk);
				}	
				wait_exit(tsk->wait_connect);
				wait_exit(tsk->wait_accept);
				wait_exit(tsk->wait_recv);
				wait_exit(tsk->wait_send);
				
				tcp_set_state(tsk, TCP_CLOSED);
				tcp_send_control_packet(tsk, TCP_RST);
			} else if (tsk->state != TCP_CLOSED) {
				printf("retrans: time %d\n", time_entry->retrans_time);
				tsk->ssthresh = max(((u32)(tsk->cwnd / 2)), 1);
				tsk->cwnd = 1;
				tsk->nr_state = TCP_LOSS;
				tsk->loss_point = tsk->snd_nxt;
				time_entry->retrans_time += 1;
				time_entry->timeout = TCP_RETRANS_INTERVAL_INITIAL * (2 << time_entry->retrans_time);
				retrans_send_buffer_packet(tsk);
			}
		}
	}
}

void *tcp_cwnd_record_thread(void *arg) {
	struct tcp_sock *tsk = (struct tcp_sock *)arg;
	FILE *fd = fopen("cwnd.csv", "w");
	double i = 0;
	while (tsk->state != TCP_TIME_WAIT) {
		usleep(50);
		i++;
		fprintf(fd, "%f,%f\n",i/10000, tsk->cwnd);
	}
	fclose(fd);
	return NULL;
}