#ifndef __BASE_H__
#define __BASE_H__

#include "types.h"
#include "ether.h"
#include "list.h"

#include <arpa/inet.h>

#define DYNAMIC_ROUTING

typedef struct {
	struct list_head iface_list;
	int nifs;
	struct pollfd *fds;
} ustack_t;

extern ustack_t *instance;

typedef struct stp_port stp_port_t;
typedef struct {
	struct list_head list;

	int fd;
	int index;
	char name[16];

	u8	mac[ETH_ALEN];

	stp_port_t *port;
} iface_info_t;

#endif
