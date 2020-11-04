#include "arpcache.h"
#include "arp.h"
#include "ether.h"
#include "packet.h"
#include "icmp.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

static arpcache_t arpcache;

// initialize IP->mac mapping, request list, lock and sweeping thread
void arpcache_init()
{
	bzero(&arpcache, sizeof(arpcache_t));

	init_list_head(&(arpcache.req_list));

	pthread_mutex_init(&arpcache.lock, NULL);

	pthread_create(&arpcache.thread, NULL, arpcache_sweep, NULL);
}

// release all the resources when exiting
void arpcache_destroy()
{
	pthread_mutex_lock(&arpcache.lock);

	struct arp_req *req_entry = NULL, *req_q;
	list_for_each_entry_safe(req_entry, req_q, &(arpcache.req_list), list) {
		struct cached_pkt *pkt_entry = NULL, *pkt_q;
		list_for_each_entry_safe(pkt_entry, pkt_q, &(req_entry->cached_packets), list) {
			list_delete_entry(&(pkt_entry->list));
			free(pkt_entry->packet);
			free(pkt_entry);
		}

		list_delete_entry(&(req_entry->list));
		free(req_entry);
	}

	pthread_kill(arpcache.thread, SIGTERM);

	pthread_mutex_unlock(&arpcache.lock);
}

// lookup the IP->mac mapping
//
// traverse the table to find whether there is an entry with the same IP
// and mac address with the given arguments
int arpcache_lookup(u32 ip4, u8 mac[ETH_ALEN])
{
	fprintf(stderr, "TODO: lookup ip address in arp cache.\n");
	pthread_mutex_lock(&arpcache.lock);
	for (int i = 0; i < MAX_ARP_SIZE; i++) {
		if (ip4 == arpcache.entries[i].ip4 && arpcache.entries[i].valid != 0) {
			memcpy(mac,arpcache.entries[i].mac,ETH_ALEN);
			pthread_mutex_unlock(&arpcache.lock);
			return 1;
		}
	}
	pthread_mutex_unlock(&arpcache.lock);
	return 0;
}

// append the packet to arpcache
//
// Lookup in the list which stores pending packets, if there is already an
// entry with the same IP address and iface (which means the corresponding arp
// request has been sent out), just append this packet at the tail of that entry
// (the entry may contain more than one packet); otherwise, malloc a new entry
// with the given IP address and iface, append the packet, and send arp request.
void arpcache_append_packet(iface_info_t *iface, u32 ip4, char *packet, int len)
{
	fprintf(stderr, "TODO: append the ip address if lookup failed, and send arp request if necessary.\n");
	pthread_mutex_lock(&arpcache.lock);
	struct arp_req *req_entry = NULL, *req_q;
	int found_same_ip_entry = 0;
	list_for_each_entry_safe(req_entry, req_q, &(arpcache.req_list), list) {
		if (req_entry->ip4 == ip4) {
			found_same_ip_entry = 1;
			break;
		}
	}
	
	if (found_same_ip_entry == 0) {
		req_entry = (struct arp_req *) safe_malloc(sizeof(struct arp_req));
		init_list_head(&(req_entry->list));
		init_list_head(&(req_entry->cached_packets));
		//req_entry->iface = iface;
		req_entry->iface = (iface_info_t *) safe_malloc(sizeof(iface_info_t));
		memcpy(req_entry->iface, iface, sizeof(iface_info_t));
		req_entry->ip4 = ip4;
		req_entry->sent = 0;
		req_entry->retries = 0;
		
		list_add_tail(&(req_entry->list), &(arpcache.req_list));
	}

	struct cached_pkt * pkt = (struct cached_pkt *) safe_malloc(sizeof(struct cached_pkt));
	pkt->packet = packet;
	pkt->len = len;
	init_list_head(&(pkt->list));
	list_add_tail(&(pkt->list), &(req_entry->cached_packets));

	arp_send_request(iface, ip4);
	req_entry->retries ++;
	req_entry->sent = time(NULL);
	pthread_mutex_unlock(&arpcache.lock);
}

// insert the IP->mac mapping into arpcache, if there are pending packets
// waiting for this mapping, fill the ethernet header for each of them, and send
// them out
void arpcache_insert(u32 ip4, u8 mac[ETH_ALEN])
{
	fprintf(stderr, "TODO: insert ip->mac entry, and send all the pending packets.\n");
	pthread_mutex_lock(&arpcache.lock);
	int found_valid_entry = 0;
	for (int i = 0; i < MAX_ARP_SIZE; i++) {
		if (arpcache.entries[i].valid != 0) {
			arpcache.entries[i].ip4 = ip4;
			memcpy(&arpcache.entries[i].mac, mac, ETH_ALEN);
			arpcache.entries[i].valid = 1;
			time_t now = time(NULL);
			arpcache.entries[i].added = now;
			found_valid_entry = 1;
			break;
		}
	}

	if (found_valid_entry == 0) {
		// replace an entry randomly
		srand(time(NULL));
		int index = rand() % MAX_ARP_SIZE;
		arpcache.entries[index].ip4 = ip4;
		memcpy(&arpcache.entries[index].mac, mac, ETH_ALEN);
		arpcache.entries[index].valid = 1;
		time_t now = time(NULL);
		arpcache.entries[index].added = now;
	}

	struct arp_req *req_entry = NULL, *req_q;
	list_for_each_entry_safe(req_entry, req_q, &(arpcache.req_list), list) {
		if (req_entry->ip4 == ip4) {
			struct cached_pkt *pkt_entry = NULL, *pkt_q;
			list_for_each_entry_safe(pkt_entry, pkt_q, &(req_entry->cached_packets), list) {
				pthread_mutex_unlock(&arpcache.lock);
				iface_send_packet_by_arp(req_entry->iface, ip4, pkt_entry->packet, pkt_entry->len);
				pthread_mutex_lock(&arpcache.lock);
				list_delete_entry(&(pkt_entry->list));
				free(pkt_entry);
			}

			list_delete_entry(&(req_entry->list));
			free(req_entry);
			break;
		}
		
	}

	pthread_mutex_unlock(&arpcache.lock);
	
}

// sweep arpcache periodically
//
// For the IP->mac entry, if the entry has been in the table for more than 15
// seconds, remove it from the table.
// For the pending packets, if the arp request is sent out 1 second ago, while 
// the reply has not been received, retransmit the arp request. If the arp
// request has been sent 5 times without receiving arp reply, for each
// pending packet, send icmp packet (DEST_HOST_UNREACHABLE), and drop these
// packets.
void *arpcache_sweep(void *arg) 
{
	while (1) {
		sleep(1);
		//fprintf(stderr, "TODO: sweep arpcache periodically: remove old entries, resend arp requests .\n");
		pthread_mutex_lock(&arpcache.lock);
		for (int i = 0; i < MAX_ARP_SIZE; i++) {
			time_t now = time(NULL);
			if ((now - arpcache.entries[i].added) > ARP_ENTRY_TIMEOUT && arpcache.entries[i].valid) {
				arpcache.entries[i].valid = 0;
			}
		}
		struct arp_req *req_entry = NULL, *req_q;
		list_for_each_entry_safe(req_entry, req_q, &(arpcache.req_list), list) {
			struct cached_pkt *pkt_entry = NULL, *pkt_q;
			time_t now = time(NULL);
			if ((now - req_entry->sent) > 1 && req_entry->retries <= ARP_REQUEST_MAX_RETRIES) {
				arp_send_request(req_entry->iface, req_entry->ip4);
				req_entry->sent = now;
				req_entry->retries ++;
			}
			if (req_entry->retries > ARP_REQUEST_MAX_RETRIES) {
				list_for_each_entry_safe(pkt_entry, pkt_q, &(req_entry->cached_packets), list) {
					pthread_mutex_unlock(&arpcache.lock);
					icmp_send_packet(pkt_entry->packet, pkt_entry->len, ICMP_DEST_UNREACH, ICMP_HOST_UNREACH);
					pthread_mutex_lock(&arpcache.lock);
					list_delete_entry(&(pkt_entry->list));
					free(pkt_entry->packet);
					free(pkt_entry);
				}
				list_delete_entry(&(req_entry->list));
				free(req_entry);
			}			
		}
		pthread_mutex_unlock(&arpcache.lock);
	}

	return NULL;
}
