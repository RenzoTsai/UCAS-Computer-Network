#include "rtable.h"
#include "ip.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct list_head rtable;

void init_rtable()
{
	init_list_head(&rtable);
}

rt_entry_t *new_rt_entry(u32 dest, u32 mask, u32 gw, iface_info_t *iface)
{
	rt_entry_t *entry = malloc(sizeof(*entry));
	memset(entry, 0, sizeof(*entry));

	init_list_head(&(entry->list));
	entry->dest = dest;
	entry->mask = mask;
	entry->gw = gw;
	entry->iface = iface;
	strcpy(entry->if_name, iface->name);

	return entry;
}

void add_rt_entry(rt_entry_t *entry)
{
	list_add_tail(&entry->list, &rtable);
}

void remove_rt_entry(rt_entry_t *entry)
{
	list_delete_entry(&entry->list);
	free(entry);
}

void clear_rtable()
{
	struct list_head *head = &rtable, *tmp;
	while (head->next != head) {
		tmp = head->next;
		list_delete_entry(tmp);
		rt_entry_t *entry = list_entry(tmp, rt_entry_t, list);
		free(entry);
	}
}

void print_rtable()
{
	// Print the route records
	fprintf(stdout, "Routing Table:\n");
	fprintf(stdout, "dest\tmask\tgateway\tif_name\n");
	fprintf(stdout, "--------------------------------------\n");
	rt_entry_t *entry = NULL;
	list_for_each_entry(entry, &rtable, list) {
		fprintf(stdout, IP_FMT"\t"IP_FMT"\t"IP_FMT"\t%s\n", \
				HOST_IP_FMT_STR(entry->dest), \
				HOST_IP_FMT_STR(entry->mask), \
				HOST_IP_FMT_STR(entry->gw), \
				entry->if_name);
	}
	fprintf(stdout, "--------------------------------------\n");
}
