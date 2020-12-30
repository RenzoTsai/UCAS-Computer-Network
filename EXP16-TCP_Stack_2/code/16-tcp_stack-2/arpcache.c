#include "arpcache.h"
#include "arp.h"
#include "ether.h"
#include "packet.h"
#include "icmp.h"
#include "ip.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

static arpcache_t arpcache;

int arpcache_lookup(u32 ip4, u8 mac[ETH_ALEN])
{
	int found = 0;

	pthread_mutex_lock(&arpcache.lock);
	for (int i = 0; i < MAX_ARP_SIZE; i++) {
		struct arp_cache_entry *entry = &arpcache.entries[i];
		if (entry->valid && entry->ip4 == ip4) {
			memcpy(mac, entry->mac, ETH_ALEN);
			found = 1;
			break;
		}
	}
	pthread_mutex_unlock(&arpcache.lock);

	return found;
}

void arpcache_append_packet(iface_info_t *iface, u32 ip4, char *packet, int len)
{
	struct cached_pkt *pkt_entry = malloc(sizeof(struct cached_pkt));
	pkt_entry->packet = packet;
	pkt_entry->len = len;

	pthread_mutex_lock(&arpcache.lock);

	int found = 0;
	struct arp_req *req_entry = NULL;
	list_for_each_entry(req_entry, &(arpcache.req_list), list) {
		if (req_entry->iface->index == iface->index && \
				req_entry->ip4 == ip4) {
			found = 1;
			break;
		}
	}

	if (!found) {
		req_entry = (struct arp_req *)malloc(sizeof(struct arp_req));
		bzero(req_entry, sizeof(struct arp_req));
		req_entry->iface = iface;
		req_entry->ip4 = ip4;
		req_entry->sent = time(NULL);
		req_entry->retries = 1;
		init_list_head(&(req_entry->cached_packets));

		list_add_head(&(req_entry->list), &(arpcache.req_list));
	}

	list_add_tail(&(pkt_entry->list), &(req_entry->cached_packets));

	pthread_mutex_unlock(&arpcache.lock);

	if (!found) {
		// last, send arp request packet
		arp_send_request(iface, ip4);
	}
}

void arpcache_insert(u32 ip4, u8 mac[ETH_ALEN])
{
	pthread_mutex_lock(&arpcache.lock);
	struct arp_cache_entry *entries = arpcache.entries;
	int pos = -1, found = 0;
	for (int i = 0; i < MAX_ARP_SIZE; i++) {
		if (entries[i].valid && entries[i].ip4 == ip4) {
			memcpy(entries[i].mac, mac, ETH_ALEN);
			entries[i].added = time(NULL);
			found = 1;
		}
		if (! entries[i].valid)
			pos = i;
	}

	if (!found) {
		if (pos != -1)
			pos = rand() % MAX_ARP_SIZE;

		entries[pos].ip4 = ip4;
		memcpy(entries[pos].mac, mac, ETH_ALEN);
		entries[pos].added = time(NULL);
		entries[pos].valid = 1;
	}

	// scan the packet list.
	struct arp_req *entry = NULL, *q;
	list_for_each_entry_safe(entry, q, &(arpcache.req_list), list) {
		if (entry->ip4 == ip4) {
			// send those packets
			struct cached_pkt *pkt = NULL, *qq;
			list_for_each_entry_safe(pkt, qq, &(entry->cached_packets), list) {
				memcpy(pkt->packet + 0, mac, ETH_ALEN);
				memcpy(pkt->packet + ETH_ALEN, entry->iface->mac, ETH_ALEN);
				iface_send_packet(entry->iface, pkt->packet, pkt->len);
				pkt->packet = NULL;

				list_delete_entry(&(pkt->list));
				free(pkt);
			}

			list_delete_entry(&(entry->list));
			free(entry);

			break;
		}
	}

	pthread_mutex_unlock(&(arpcache.lock));
}

void *arpcache_sweep(void *arg) 
{
	while (1) {
		sleep(1);
		time_t now = time(NULL);

		pthread_mutex_lock(&arpcache.lock);
	
		struct arp_cache_entry *entries = arpcache.entries;
		for (int i = 0; i < MAX_ARP_SIZE; i++) {
			if (entries[i].valid && now - entries[i].added >= ARP_ENTRY_TIMEOUT) 
				entries[i].valid = 0;
		}

		struct list_head pkt_list;
		init_list_head(&pkt_list);

		struct arp_req *req_entry = NULL, *req_q;
		list_for_each_entry_safe(req_entry, req_q, &(arpcache.req_list), list) {
			if (now - req_entry->sent < 1)
				continue;

			if (req_entry->retries < ARP_REQUEST_MAX_RETRIES) {
				// resend the arp request
				req_entry->retries += 1;
				arp_send_request(req_entry->iface, req_entry->ip4);

				continue;
			}
	
			struct cached_pkt *pkt_entry = NULL, *pkt_q;
			list_for_each_entry_safe(pkt_entry, pkt_q, &(req_entry->cached_packets), list) {
				list_delete_entry(&(pkt_entry->list));
				list_add_tail(&(pkt_entry->list), &pkt_list);
			}
	
			list_delete_entry(&(req_entry->list));
			free(req_entry);
		}
	
		pthread_mutex_unlock(&arpcache.lock);

		struct cached_pkt *pkt_entry = NULL, *pkt_q;
		list_for_each_entry_safe(pkt_entry, pkt_q, &pkt_list, list) {
			list_delete_entry(&(pkt_entry->list));
			icmp_send_packet(pkt_entry->packet, pkt_entry->len, \
					ICMP_DEST_UNREACH, ICMP_HOST_UNREACH);
			free(pkt_entry->packet);
			free(pkt_entry);
		}
	}

	return NULL;
}

void arpcache_init()
{
	bzero(&arpcache, sizeof(arpcache_t));

	init_list_head(&(arpcache.req_list));

	pthread_mutex_init(&arpcache.lock, NULL);

	pthread_create(&arpcache.thread, NULL, arpcache_sweep, NULL);
}

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
