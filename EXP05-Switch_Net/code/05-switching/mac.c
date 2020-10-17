#include "mac.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>
#include <utils.h>

mac_port_map_t mac_port_map;

// initialize mac_port table
void init_mac_port_table()
{
	bzero(&mac_port_map, sizeof(mac_port_map_t));

	for (int i = 0; i < HASH_8BITS; i++) {
		init_list_head(&mac_port_map.hash_table[i]);
	}

	pthread_mutex_init(&mac_port_map.lock, NULL);

	pthread_create(&mac_port_map.thread, NULL, sweeping_mac_port_thread, NULL);
}

// destroy mac_port table
void destory_mac_port_table()
{
	pthread_mutex_lock(&mac_port_map.lock);
	mac_port_entry_t *entry, *q;
	for (int i = 0; i < HASH_8BITS; i++) {
		list_for_each_entry_safe(entry, q, &mac_port_map.hash_table[i], list) {
			list_delete_entry(&entry->list);
			free(entry);
		}
	}
	pthread_mutex_unlock(&mac_port_map.lock);
}

// compare mac
int mac_cmp (u8* mac_1, u8* mac_2 ,int len) {
	for (int i = 0; i < len; i++) {
		if (mac_1[i] != mac_2[i]) {
			return i+1;
		}
	}
	return 0;
}

// lookup the mac address in mac_port table
iface_info_t *lookup_port(u8 mac[ETH_ALEN])
{
	// TODO: implement the lookup process here
	//fprintf(stdout, "TODO: implement the lookup process here.\n");
	pthread_mutex_lock(&mac_port_map.lock);
	u8 hash_value = hash8((char *)mac, ETH_ALEN);
	mac_port_entry_t * mac_port_entry_pos = NULL;
	
	list_for_each_entry(mac_port_entry_pos, &mac_port_map.hash_table[hash_value], list) {
		if (mac_cmp(mac_port_entry_pos->mac, mac, ETH_ALEN) == 0) {
			mac_port_entry_pos->visited = time(NULL);
			pthread_mutex_unlock(&mac_port_map.lock);
			return mac_port_entry_pos->iface;
		}
	}
	pthread_mutex_unlock(&mac_port_map.lock);

	return NULL;
}

// copy mac from src to dst
int mac_cpy (u8* dst, u8* src, int len) {
	for (int i = 0; i < len; i++) {
		dst[i] = src[i];
	}
	return 0;
}

// insert the mac -> iface mapping into mac_port table
void insert_mac_port(u8 mac[ETH_ALEN], iface_info_t *iface)
{
	// TODO: implement the insertion process here
	// fprintf(stdout, "TODO: implement the insertion process here.\n");
	pthread_mutex_lock(&mac_port_map.lock);
	mac_port_entry_t * new_mac_port_entry = safe_malloc(sizeof(mac_port_entry_t));
	bzero(new_mac_port_entry, sizeof(mac_port_entry_t));
	mac_cpy(new_mac_port_entry->mac, mac, ETH_ALEN);
	new_mac_port_entry->iface = iface;
	new_mac_port_entry->visited = time(NULL);
	u8 hash_value = hash8((char *)mac, ETH_ALEN);
	list_add_tail(&new_mac_port_entry->list, &mac_port_map.hash_table[hash_value]);
	pthread_mutex_unlock(&mac_port_map.lock);
}

// dumping mac_port table
void dump_mac_port_table()
{
	mac_port_entry_t *entry = NULL;
	time_t now = time(NULL);

	// fprintf(stdout, "dumping the mac_port table:\n");
	pthread_mutex_lock(&mac_port_map.lock);
	for (int i = 0; i < HASH_8BITS; i++) {
		list_for_each_entry(entry, &mac_port_map.hash_table[i], list) {
			fprintf(stdout, ETHER_STRING " -> %s, %d\n", ETHER_FMT(entry->mac), \
					entry->iface->name, (int)(now - entry->visited));
		}
	}

	pthread_mutex_unlock(&mac_port_map.lock);
}

// sweeping mac_port table, remove the entry which has not been visited in the
// last 30 seconds.
int sweep_aged_mac_port_entry()
{
	// TODO: implement the sweeping process here
	// fprintf(stdout, "TODO: implement the sweeping process here.\n");
	pthread_mutex_lock(&mac_port_map.lock);	
	mac_port_entry_t *entry = NULL;
	mac_port_entry_t *q = NULL;
	time_t now = time(NULL);
	int rm_entry_num = 0;
	for (int i = 0; i < HASH_8BITS; i++) {
		list_for_each_entry_safe(entry, q, &mac_port_map.hash_table[i], list) {
			if ((int)(now - entry->visited) > MAC_PORT_TIMEOUT) {
				list_delete_entry(&entry->list);
				free(entry);
				rm_entry_num ++;
			}
		}
	}
	pthread_mutex_unlock(&mac_port_map.lock);
	return rm_entry_num;
}

// sweeping mac_port table periodically, by calling sweep_aged_mac_port_entry
void *sweeping_mac_port_thread(void *nil)
{
	while (1) {
		sleep(1);
		int n = sweep_aged_mac_port_entry();

		if (n > 0)
			log(DEBUG, "%d aged entries in mac_port table are removed.", n);
	}

	return NULL;
}
