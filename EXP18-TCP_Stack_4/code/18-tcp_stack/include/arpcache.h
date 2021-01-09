#ifndef __ARPCACHE_H__
#define __ARPCACHE_H__

#include "base.h"
#include "types.h"
#include "list.h"

#include <pthread.h>

#define MAX_ARP_SIZE 32				// maximum number of IP->mac mapping entries
#define ARP_ENTRY_TIMEOUT 15		// timeout for IP->mac entry
#define ARP_REQUEST_MAX_RETRIES	5	// maximum number of retries of arp request

// pending packet, waiting for arp reply
struct cached_pkt {
	struct list_head list;
	char *packet;			// packet
	int len;				// the length of packet
};

// list of pending packets, with the same iface and destination ip address
struct arp_req {
	struct list_head list;	
	iface_info_t *iface;	// the interface that will send the pending packets
	u32 ip4;				// destination ip address
	time_t sent;			// last time when arp request is sent
	int retries;			// number of retries
	struct list_head cached_packets;	// pending packets
};

struct arp_cache_entry {
	u32 ip4; 			// destination ip address, stored in host byte order
	u8 mac[ETH_ALEN];	// mac address
	time_t added;		// the time when this entry is inserted
	int valid;			// whether this entry is valid (has not triggered the timeout)
};

typedef struct {
	struct arp_cache_entry entries[MAX_ARP_SIZE];	// IP->max mapping entries
	struct list_head req_list;			// the pending packet list
	pthread_mutex_t lock;				// each operation on arp cache should apply the lock first
	pthread_t thread;					// the id of the arp cache sweeping thread
} arpcache_t;

void arpcache_init();
void arpcache_destroy();

int arpcache_lookup(u32 ip4, u8 mac[]);
void arpcache_insert(u32 ip4, u8 mac[]);
void arpcache_append_packet(iface_info_t *iface, u32 ip4, char *packet, int len);

#endif
