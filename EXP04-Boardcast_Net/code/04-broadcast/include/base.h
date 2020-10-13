#ifndef __BASE_H__
#define __BASE_H__

#include "types.h"
#include "ether.h"
#include "list.h"

#include <arpa/inet.h>

typedef struct {
	struct list_head iface_list;
	int nifs;
	struct pollfd *fds;
} ustack_t;

extern ustack_t *instance;

typedef struct {
	struct list_head list;

	int fd;
	int index;
	u8	mac[ETH_ALEN];
	char name[16];
} iface_info_t;

iface_info_t *if_name_to_iface(const char *if_name);

#endif
