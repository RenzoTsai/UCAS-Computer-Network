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
	tsk->snd_wnd = cb->rwnd;
	if (old_snd_wnd == 0) {
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
		return;
	}
	printf("handle recv data\n");
	add_recv_ofo_buf_entry(tsk, cb);
	put_recv_ofo_buf_entry_to_ring_buf(tsk);
	tsk->snd_una = (greater_than_32b(cb->ack, tsk->snd_una))?cb->ack :tsk->snd_una;
	tcp_update_retrans_timer(tsk);
	tcp_send_control_packet(tsk, TCP_ACK);
}


// Process the incoming packet according to TCP state machine. 
void tcp_process(struct tcp_sock *tsk, struct tcp_cb *cb, char *packet)
{
	struct tcphdr * tcp = packet_to_tcp_hdr(packet);
	
	printf("flags: 0x%x\n",tcp->flags);
	printf("state: %s\n",tcp_state_to_str(tsk->state));
	printf("rcv_nxt:%u, ack:%u, seq:%u\n", tsk->rcv_nxt, cb->ack, cb->seq);

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
			tcp_unset_retrans_timer(tsk);
			tcp_set_state(tsk, TCP_CLOSE_WAIT);
			tsk->rcv_nxt = cb->seq + 1;
			tcp_send_control_packet(tsk, TCP_ACK);
		} else if (tcp->flags & TCP_ACK) {
			if (!is_tcp_seq_valid(tsk,cb)) {
				return;
			}
			if (cb->pl_len == 0) {
				tsk->rcv_nxt = cb->seq;
				tsk->snd_una = cb->ack;
				tcp_update_window_safe(tsk, cb);
				delete_send_buffer_entry(tsk, cb->ack);
				tcp_update_retrans_timer(tsk);
			} else {
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
