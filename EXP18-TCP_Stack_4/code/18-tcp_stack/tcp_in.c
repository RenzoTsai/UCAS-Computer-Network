#include "tcp.h"
#include "tcp_sock.h"
#include "tcp_timer.h"

#include "log.h"
#include "ring_buffer.h"

#include <stdlib.h>
// update the snd_wnd of tcp_sock
//
// if the snd_wnd before updating is zero, notify tcp_sock_send (wait_send)
static inline void tcp_update_window(struct tcp_sock *tsk, struct tcp_cb *cb)
{
	u16 old_snd_wnd = tsk->snd_wnd;
	//tsk->snd_wnd = cb->rwnd;
	tsk->snd_wnd = min(cb->rwnd, tsk->cwnd * MSS);
	if ((int)old_snd_wnd <= 0) {
		wake_up(tsk->wait_send);
		// printf("update windows\n");
	}
}

// update the snd_wnd safely: cb->ack should be between snd_una and snd_nxt
static inline void tcp_update_window_safe(struct tcp_sock *tsk, struct tcp_cb *cb)
{
	if (less_or_equal_32b(tsk->snd_una, cb->ack) && less_or_equal_32b(cb->ack, tsk->snd_nxt))
		tcp_update_window(tsk, cb);
}

#ifndef max
#	define max(x,y) ((x)>(y) ? (x) : (y))
#endif

// check whether the sequence number of the incoming packet is in the receiving
// window
static inline int is_tcp_seq_valid(struct tcp_sock *tsk, struct tcp_cb *cb)
{
	u32 rcv_end = tsk->rcv_nxt + max(tsk->rcv_wnd, 1);
	if (less_than_32b(cb->seq, rcv_end) && less_or_equal_32b(tsk->rcv_nxt, cb->seq_end)) {
		return 1;
	}
	else {
		log(ERROR, "received packet with invalid seq, drop it.");
		return 0;
	}
}

struct tcp_sock * alloc_child_tcp_sock(struct tcp_sock *tsk, struct tcp_cb *cb) {
	struct tcp_sock * child = alloc_tcp_sock();
	memcpy((char*)child, (char*)tsk, sizeof(struct tcp_sock));
	child->parent = tsk;
	child->sk_sip = cb->daddr;
	child->sk_sport = cb->dport;
	child->sk_dip = cb->saddr;
	child->sk_dport = cb->sport;
	child->iss = tcp_new_iss();
	child->snd_nxt = child->iss;
	child->rcv_nxt = cb->seq + 1;
	init_list_head(&child->send_buf);
	init_list_head(&child->rcv_ofo_buf);
	tcp_sock_listen_enqueue(child);
	tcp_hash(child);
	return child;
}

void handle_recv_data(struct tcp_sock *tsk, struct tcp_cb *cb) {
	while (ring_buffer_full(tsk->rcv_buf)) {
		sleep_on(tsk->wait_recv);
	}
	if (less_than_32b(cb->seq, tsk->rcv_nxt)) {
		tcp_send_control_packet(tsk, TCP_ACK);
		return;
	}
	//printf("handle recv data\n");
	add_recv_ofo_buf_entry(tsk, cb);
	put_recv_ofo_buf_entry_to_ring_buf(tsk);
	//tcp_send_control_packet(tsk, TCP_ACK);
	tsk->snd_una = (greater_than_32b(cb->ack, tsk->snd_una))?cb->ack :tsk->snd_una;
	tcp_update_retrans_timer(tsk);
	
}

void tcp_new_reno_process(struct tcp_sock *tsk, struct tcp_cb *cb, char *packet) {
	int isNewAck  = delete_send_buffer_entry(tsk, cb->ack);
	printf("***********RENO**********");
	if (isNewAck) {
		//tsk->dupacks --;
		if (tsk->nr_state == TCP_OPEN 	  || tsk->nr_state == TCP_DISORDER 
		 || tsk->nr_state == TCP_RECOVERY || tsk->nr_state == TCP_LOSS) {
			if ((int)tsk->cwnd < tsk->ssthresh) {
				tsk->cwnd ++;
				printf("cwnd+1:%f\n",tsk->cwnd);
			} else {
				tsk->cwnd += 1.0/tsk->cwnd;
				printf("cwnd+1/:%f\n",tsk->cwnd);
			}
			if (tsk->nr_state != TCP_LOSS || cb->ack >= tsk->loss_point) {
				tsk->nr_state = TCP_OPEN;
			}
		} else if (tsk->nr_state == TCP_FR) {
			if (tsk->cwnd_flag == 1) {
				tsk->cwnd += 1.0/tsk->cwnd;
				printf("cwnd+1:%f\n",tsk->cwnd);
			} else if (tsk->cwnd > tsk->ssthresh) {
				tsk->cwnd -= 0.5;
				printf("cwnd-:%f\n",tsk->cwnd);
			} else {
				tsk->cwnd_flag = 1;
			}       
			if (cb->ack < tsk->recovery_point) {
				retrans_send_buffer_packet(tsk);
			} else {
				tsk->nr_state = TCP_OPEN;
				tsk->dupacks = 0;
			}
		}
	} else {
		tsk->dupacks ++;
		if (tsk->nr_state == TCP_OPEN || tsk->nr_state == TCP_DISORDER || tsk->nr_state == TCP_LOSS) {
			if ((int)tsk->cwnd < tsk->ssthresh) {
				tsk->cwnd ++;
				printf("cwnd+1:%f\n",tsk->cwnd);
			} else {
				tsk->cwnd += 1.0/tsk->cwnd;
				printf("cwnd+1/:%f\n",tsk->cwnd);
			}
			if (tsk->nr_state == TCP_OPEN) {
				tsk->nr_state = TCP_DISORDER;
			} else if (tsk->nr_state == TCP_DISORDER) {
				tsk->nr_state = TCP_RECOVERY;
			}
		} else {
			if (tsk->nr_state == TCP_RECOVERY) {
				if (tsk->dupacks == 3) {
					tsk->ssthresh = max((u32)(tsk->cwnd / 2), 1);
					tsk->cwnd -= 0.5;
					//tsk->cwnd = tsk->ssthresh;
					printf("cwnd-:%f\n",tsk->cwnd);
					tsk->cwnd_flag = 0;
					tsk->recovery_point = tsk->snd_nxt;
					retrans_send_buffer_packet(tsk);
					tsk->nr_state = TCP_FR;
				}
			}
			if (tsk->nr_state == TCP_FR) {
				if (tsk->cwnd_flag == 1) {
					tsk->cwnd += (1.0/tsk->cwnd);
					printf("cwnd+1/:%f\n",tsk->cwnd);
				} else if (tsk->cwnd > tsk->ssthresh) {
					tsk->cwnd -= 0.5;
					printf("cwnd-:%f\n",tsk->cwnd);
				} else {
					tsk->cwnd_flag = 1;
				}
			}
		}
	}
}

// Process the incoming packet according to TCP state machine. 
void tcp_process(struct tcp_sock *tsk, struct tcp_cb *cb, char *packet)
{
	struct tcphdr * tcp = packet_to_tcp_hdr(packet);
	
	printf("flags: 0x%x\n",tcp->flags);
	printf("state: %s\n",tcp_state_to_str(tsk->state));
	printf("rcv_nxt:%u, ack:%u, seq:%u, len:%d\n", tsk->rcv_nxt, cb->ack, cb->seq, cb->pl_len);
	tsk->adv_wnd = cb->rwnd;

	if (tcp->flags & TCP_RST) {
		tcp_sock_close(tsk);
		return;
	}

	if (tsk->state == TCP_LISTEN) {
		if (tcp->flags & TCP_SYN) {
			tcp_set_state(tsk, TCP_SYN_RECV);
			struct tcp_sock * child = alloc_child_tcp_sock(tsk, cb);
			tcp_set_retrans_timer(child);
			tcp_send_control_packet(child, TCP_ACK|TCP_SYN);
		}
		return;
	}

	if (tsk->state == TCP_SYN_SENT) {
		if ((tcp->flags & (TCP_SYN|TCP_ACK)) == (TCP_SYN|TCP_ACK)) {
			tsk->rcv_nxt = cb->seq + 1;
		    tsk->snd_una = cb->ack;
			delete_send_buffer_entry(tsk, cb->ack);
			tcp_unset_retrans_timer(tsk);
			tcp_set_state(tsk, TCP_ESTABLISHED);
			// tsk->nr_state = TCP_OPEN;
			// tsk->cwnd = 1;
			// tsk->ssthresh = 16;
			tcp_send_control_packet(tsk, TCP_ACK);	
			wake_up(tsk->wait_connect);	
		}
		return;
	}

	if (tsk->state == TCP_SYN_RECV) {
		if (tcp->flags & TCP_ACK) {
			if (!tcp_sock_accept_queue_full(tsk->parent)) {
				if (!is_tcp_seq_valid(tsk, cb)) {
					return;
				}
				tsk->rcv_nxt = cb->seq;
		        tsk->snd_una = cb->ack;
				delete_send_buffer_entry(tsk, cb->ack);
				tcp_unset_retrans_timer(tsk);
				tcp_sock_accept_enqueue(tsk);
				wake_up(tsk->parent->wait_accept);
			}
		}
	}

	if (tsk->state == TCP_ESTABLISHED) {
		if (tcp->flags & TCP_FIN) {
			if (!is_tcp_seq_valid(tsk,cb)) {
				return;
			}
			tcp_unset_retrans_timer(tsk);
			tcp_set_state(tsk, TCP_CLOSE_WAIT);
			tsk->rcv_nxt = cb->seq + 1;
			tcp_send_control_packet(tsk, TCP_ACK);
		} else if (tcp->flags & TCP_ACK) {
			if (cb->pl_len == 0) {
				// for sender
				tsk->rcv_nxt = cb->seq;
				tsk->snd_una = cb->ack;
				tcp_update_window_safe(tsk, cb);
				tcp_update_retrans_timer(tsk);
				//delete_send_buffer_entry(tsk, cb->ack);
				tcp_new_reno_process(tsk, cb, packet);
				wake_up(tsk->wait_send);
			} else {
				// for receiver
				handle_recv_data(tsk, cb);
			}	
		}

	}

	if (tsk->state == TCP_LAST_ACK) {
		if (tcp->flags & TCP_ACK) {
			if (!is_tcp_seq_valid(tsk,cb)) {
				return;
			}
			delete_send_buffer_entry(tsk, cb->ack);
			tcp_unset_retrans_timer(tsk);
			tcp_set_state(tsk, TCP_CLOSED);
			tcp_unhash(tsk);
		}
	}

	if (tsk->state == TCP_FIN_WAIT_1) {
		if (tcp->flags & TCP_ACK) {
			if (!is_tcp_seq_valid(tsk,cb)) {
				return;
			}
			delete_send_buffer_entry(tsk, cb->ack);
			tcp_unset_retrans_timer(tsk);
			tcp_set_state(tsk, TCP_FIN_WAIT_2);
		}
	}

	if (tsk->state == TCP_FIN_WAIT_2) {
		if (tcp->flags & TCP_FIN) {
			tcp_unset_retrans_timer(tsk);
			tsk->rcv_nxt = cb->seq + 1;
			tcp_send_control_packet(tsk, TCP_ACK);
			tcp_set_state(tsk, TCP_TIME_WAIT);
			tcp_set_timewait_timer(tsk);
		}

	}


}
