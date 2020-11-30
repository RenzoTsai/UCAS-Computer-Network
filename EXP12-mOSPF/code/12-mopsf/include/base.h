#ifndef __BASE_H__
#define __BASE_H__

#include "types.h"
#include "ether.h"
#include "list.h"

#include <arpa/inet.h>

#define DYNAMIC_ROUTING

typedef struct {
	struct list_head iface_list;	// the list of interfaces
	int nifs;						// number of interfaces
	struct pollfd *fds;				// structure used to poll packets among 
								    // all the interfaces

#ifdef DYNAMIC_ROUTING
	// used for mospf routing
	u32 area_id;	
	u32 router_id;
	u16 sequence_num;
	int lsuint;
#endif
} ustack_t;

extern ustack_t *instance;

typedef struct {
	struct list_head list;		// list node used to link all interfaces

	int fd;						// file descriptor for receiving & sending 
	                            // packets 
	int index;					// the index (unique ID) of this interface
	u8	mac[ETH_ALEN];			// mac address of this interface
	u32 ip;						// ip address of this interface
	u32 mask;					// ip mask of this interface
	char name[16];				// name of this interface
	char ip_str[16];			// string of the ip address

#ifdef DYNAMIC_ROUTING
	// list of mospf neighbors
	int helloint;
	int num_nbr;
	struct list_head nbr_list;
#endif
} iface_info_t;

#endif
