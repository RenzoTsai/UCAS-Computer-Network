#include "include/list.h"

#include <stdio.h>
#include <stdlib.h>

struct listnode {
	struct list_head list;
	int number;
};

void list_example()
{
	struct list_head list;
	init_list_head(&list);

	for (int i = 0; i < 10; i++) {
		struct listnode *node = malloc(sizeof(struct listnode));
		node->number = i;
		list_add_tail(&node->list, &list);
	}

	fprintf(stdout, "list all numbers:\n");
	struct listnode *entry;
	list_for_each_entry(entry, &list, list) {
		fprintf(stdout, "%d\n", entry->number);
	}

	fprintf(stdout, "list only odd numbers and remove others:\n");
	struct listnode *q;
	list_for_each_entry_safe(entry, q, &list, list) {
		if (entry->number % 2 == 0) {
			list_delete_entry(&entry->list);
			free(entry);
		}
		else {
			fprintf(stdout, "%d\n", entry->number);
		}
	}
}

void main()
{
	list_example();
}
