#ifndef __ARPCACHE_H__
#define __ARPCACHE_H__

#include "base.h"
#include "types.h"
#include "list.h"

#include <pthread.h>

#define MAX_ARP_SIZE 32
#define ARP_ENTRY_TIMEOUT 15
#define ARP_REQUEST_MAX_RETRIES	5

struct cached_pkt {
	struct list_head list;
	char *packet;
	int len;
};

struct arp_req {
	struct list_head list;
	iface_info_t *iface;
	u32 ip4;
	time_t sent;
	int retries;
	struct list_head cached_packets;
};

struct arp_cache_entry {
	u32 ip4; 	// stored in host byte order
	u8 mac[ETH_ALEN];
	time_t added;
	int valid;
};

typedef struct {
	struct arp_cache_entry entries[MAX_ARP_SIZE];
	struct list_head req_list;
	pthread_mutex_t lock;
	pthread_t thread;
} arpcache_t;

void arpcache_init();
void arpcache_destroy();
void *arpcache_sweep(void *);

int arpcache_lookup(u32 ip4, u8 mac[]);
void arpcache_insert(u32 ip4, u8 mac[]);
void arpcache_append_packet(iface_info_t *iface, u32 ip4, char *packet, int len);

#endif
