#ifndef __LIST_H__
#define __LIST_H__

#include <stddef.h>

// list node to link the *real* node into list
struct list_head {
	struct list_head *next, *prev;
};

// check whether the list is empty (contains only one pseudo list node)
#define list_empty(list) ((list)->next == (list))

// get the *real* node from the list node
#define list_entry(ptr, type, member) \
		(type *)((char *)ptr - offsetof(type, member))

// iterate the list 
#define list_for_each_entry(pos, head, member) \
    for (pos = list_entry((head)->next, typeof(*pos), member); \
        	&pos->member != (head); \
        	pos = list_entry(pos->member.next, typeof(*pos), member)) 

// iterate the list safely, during which node could be added or removed in the list
#define list_for_each_entry_safe(pos, q, head, member) \
    for (pos = list_entry((head)->next, typeof(*pos), member), \
	        q = list_entry(pos->member.next, typeof(*pos), member); \
	        &pos->member != (head); \
	        pos = q, q = list_entry(pos->member.next, typeof(*q), member))

// initialize the list head
static inline void init_list_head(struct list_head *list)
{
	list->next = list->prev = list;
}

// insert a new node between prev and next
static inline void list_insert(struct list_head *new,
			      struct list_head *prev,
			      struct list_head *next)
{
	next->prev = new;
	prev->next = new;
	new->next = next;
	new->prev = prev;
}

// add a list node at the head of the list
static inline void list_add_head(struct list_head *new, struct list_head *head)
{
	list_insert(new, head, head->next);
}

// add a list node at the tail of the list 
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	list_insert(new, head->prev, head);
}

// delete the node from the list (note that it only remove the entry from 
// list, but not free allocated memory)
static inline void list_delete_entry(struct list_head *entry)
{
	entry->next->prev = entry->prev;
	entry->prev->next = entry->next;
}

#endif
