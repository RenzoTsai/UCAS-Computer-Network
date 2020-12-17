#ifndef __TCP_SOCK_H__
#define __TCP_SOCK_H__

#include "types.h"
#include "list.h"
#include "tcp.h"
#include "tcp_timer.h"
#include "ring_buffer.h"

#include "synch_wait.h"

#include <pthread.h>

#define PORT_MIN	12345
#define PORT_MAX	23456

struct sock_addr {
	u32 ip;
	u16 port;
} __attribute__((packed));

// the main structure that manages a connection locally
struct tcp_sock {
	// sk_ip, sk_sport, sk_sip, sk_dport are the 4-tuple that represents a 
	// connection
	struct sock_addr local;
	struct sock_addr peer;
#define sk_sip local.ip
#define sk_sport local.port
#define sk_dip peer.ip
#define sk_dport peer.port

	// pointer to parent tcp sock, a tcp sock which bind and listen to a port 
	// is the parent of tcp socks when *accept* a connection request
	struct tcp_sock *parent;

	// represents the number that the tcp sock is referred, if this number 
	// decreased to zero, the tcp sock should be released
	int ref_cnt;

	// hash_list is used to hash tcp sock into listen_table or established_table, 
	// bind_hash_list is used to hash into bind_table
	struct list_head hash_list;
	struct list_head bind_hash_list;

	// when a passively opened tcp sock receives a SYN packet, it mallocs a child 
	// tcp sock to serve the incoming connection, which is pending in the 
	// listen_queue of parent tcp sock
	struct list_head listen_queue;
	// when receiving the last packet (ACK) of the 3-way handshake, the tcp sock 
	// in listen_queue will be moved into accept_queue, waiting for *accept* by 
	// parent tcp sock
	struct list_head accept_queue;


#define TCP_MAX_BACKLOG 128
	// the number of pending tcp sock in accept_queue
	int accept_backlog;
	// the maximum number of pending tcp sock in accept_queue
	int backlog;

	// the list node used to link listen_queue or accept_queue of parent tcp sock
	struct list_head list;
	// tcp timer used during TCP_TIME_WAIT state
	struct tcp_timer timewait;

	// used for timeout retransmission
	struct tcp_timer retrans_timer;

	// synch waiting structure of *connect*, *accept*, *recv*, and *send*
	struct synch_wait *wait_connect;
	struct synch_wait *wait_accept;
	struct synch_wait *wait_recv;
	struct synch_wait *wait_send;

	// receiving buffer
	struct ring_buffer *rcv_buf;
	// used to pend unacked packets
	struct list_head send_buf;
	// used to pend out-of-order packets
	struct list_head rcv_ofo_buf;

	// tcp state, see enum tcp_state in tcp.h
	int state;

	// initial sending sequence number
	u32 iss;

	// the highest byte that is ACKed by peer
	u32 snd_una;
	// the highest byte sent
	u32 snd_nxt;

	// the highest byte ACKed by itself (i.e. the byte expected to receive next)
	u32 rcv_nxt;

	// used to indicate the end of fast recovery
	u32 recovery_point;		

	// min(adv_wnd, cwnd)
	u32 snd_wnd;
	// the receiving window advertised by peer
	u16 adv_wnd;

	// the size of receiving window (advertised by tcp sock itself)
	u16 rcv_wnd;

	// congestion window
	u32 cwnd;

	// slow start threshold
	u32 ssthresh;
};

void tcp_set_state(struct tcp_sock *tsk, int state);

int tcp_sock_accept_queue_full(struct tcp_sock *tsk);
void tcp_sock_accept_enqueue(struct tcp_sock *tsk);
struct tcp_sock *tcp_sock_accept_dequeue(struct tcp_sock *tsk);

int tcp_hash(struct tcp_sock *tsk);
void tcp_unhash(struct tcp_sock *tsk);
void tcp_bind_unhash(struct tcp_sock *tsk);
struct tcp_sock *alloc_tcp_sock();
void free_tcp_sock(struct tcp_sock *tsk);
struct tcp_sock *tcp_sock_lookup(struct tcp_cb *cb);

u32 tcp_new_iss();

void tcp_send_reset(struct tcp_cb *cb);

void tcp_send_control_packet(struct tcp_sock *tsk, u8 flags);
void tcp_send_packet(struct tcp_sock *tsk, char *packet, int len);
int tcp_send_data(struct tcp_sock *tsk, char *buf, int len);

void tcp_process(struct tcp_sock *tsk, struct tcp_cb *cb, char *packet);

void init_tcp_stack();

int tcp_sock_bind(struct tcp_sock *tsk, struct sock_addr *skaddr);
int tcp_sock_listen(struct tcp_sock *tsk, int backlog);
int tcp_sock_connect(struct tcp_sock *tsk, struct sock_addr *skaddr);
struct tcp_sock *tcp_sock_accept(struct tcp_sock *tsk);
void tcp_sock_close(struct tcp_sock *tsk);

int tcp_sock_read(struct tcp_sock *tsk, char *buf, int len);
int tcp_sock_write(struct tcp_sock *tsk, char *buf, int len);

#endif
