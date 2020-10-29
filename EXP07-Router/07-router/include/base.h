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
	u32 ip;
	u32 mask;
	char name[16];
	char ip_str[16];
} iface_info_t;

#endif
