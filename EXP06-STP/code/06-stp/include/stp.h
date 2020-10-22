#ifndef __STP_H__
#define __STP_H__

#include "stp_proto.h"
#include "stp_timer.h"

#include "base.h"
#include "types.h"

#define STP_MAX_PORTS 32

extern const u8 eth_stp_addr[];

typedef struct stp stp_t;
struct stp_port {
	stp_t *stp;					// pointer to stp

	int port_id;				// port id
	char *port_name;
	iface_info_t *iface;

	int path_cost;				// cost of this port, always be 1 in this lab

	u64 designated_root;		// root switch (the port believes)
	u64 designated_switch;		// the switch sending this config
	int designated_port;		// the port sending this config
	int designated_cost;		// path cost to root on port
};

struct stp {
	u64 switch_id;

	u64 designated_root;		// switch root (it believes)
	int root_path_cost;			// cost of path to root
	stp_port_t *root_port;		// lowest cost port to root

	long long int last_tick;	// switch timers

	stp_timer_t hello_timer;	// hello timer

	// ports
	int nports;
	stp_port_t ports[STP_MAX_PORTS];

	pthread_mutex_t lock;
	pthread_t timer_thread;
};

void stp_init(struct list_head *iface_list);
void stp_destroy();

void stp_port_handle_packet(stp_port_t *, char *packet, int pkt_len);

#endif
