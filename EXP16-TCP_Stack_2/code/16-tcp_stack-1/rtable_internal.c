#include "rtable.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <net/if.h>
#include <net/route.h>
#include <sys/ioctl.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#define ROUTE_BATCH_SIZE 10240

// Structure for sending the request for routing table
typedef struct {
	struct nlmsghdr nlmsg_hdr;
	struct rtmsg rt_msg;
	char buf[ROUTE_BATCH_SIZE];
} route_request;

// XXX: All the functions in this file should be treated as a blackbox. You do not 
// need to understand how it works, but only trust it will process like the function
// name indicates.

static void if_index_to_name(int if_index, char *if_name)
{
	int fd = socket(AF_INET, SOCK_DGRAM, 0); 
	if (fd == -1) {
		perror("Create socket failed.");
		exit(-1);
	}

	struct ifreq ifr;
	ifr.ifr_ifindex = if_index;

	if (ioctl(fd, SIOCGIFNAME, &ifr, sizeof(ifr))) {
		perror("Get interface name failed.");
		exit(-1);
	}

	strcpy(if_name, ifr.ifr_name);
}

static iface_info_t *if_name_to_iface(const char *if_name)
{
	iface_info_t *iface = NULL;
	list_for_each_entry(iface, &instance->iface_list, list) {
		if (strcmp(iface->name, if_name) == 0)
			return iface;
	}

	fprintf(stderr, "Could not find the desired interface "
			"according to if_name '%s'\n", if_name);
	return NULL;
}

static int get_unparsed_route_info(char *buf, int size)
{
	int fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE); 

	route_request *req = (route_request *)malloc(sizeof(route_request));
	bzero(req,sizeof(route_request));
	req->nlmsg_hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
	req->nlmsg_hdr.nlmsg_type = RTM_GETROUTE;
	req->nlmsg_hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	req->rt_msg.rtm_family = AF_INET;
	req->rt_msg.rtm_table = 254; 

	if ((send(fd, req, sizeof(route_request), 0)) < 0) {
		perror("Send routing request failed.");
		exit(-1);
	}

	int len = 0;
	int left = size;
	while (1) {
		if (left < sizeof(struct nlmsghdr)) {
			fprintf(stderr, "Routing table is larger than %d.\n", size);
			exit(-1);
		}

		int nbytes = recv(fd, buf + len, left, 0);
		if (nbytes < 0) {
			fprintf(stderr, "Receive routing info failed.\n");
			break;
		} else if (nbytes == 0) {
			fprintf(stdout, "EOF in netlink\n");
			break;
		}

		struct nlmsghdr *nlp = (struct nlmsghdr*)(buf+len);
		if (nlp->nlmsg_type == NLMSG_DONE) {
			break;
		} else if (nlp->nlmsg_type == NLMSG_ERROR) {
			fprintf(stderr, "Error exists in netlink msg.\n");
			exit(-1);
		}

		len += nbytes;
		left -= nbytes;
	}

	close(fd);

	return len;
}

static int parse_routing_info(char *buf, int len)
{
	int n = 0;
	init_list_head(&rtable);

	// Outer loop: Iterate all the NETLINK headers
	for (struct nlmsghdr *nlp = (struct nlmsghdr *)buf;
			NLMSG_OK(nlp, len); nlp = NLMSG_NEXT(nlp, len)) {
		// get route entry header
		struct rtmsg *rtp = (struct rtmsg *)NLMSG_DATA(nlp);
		// we only care about the tableId route table
		if (rtp->rtm_table != 254)
			continue;

		u32 dest = 0, mask = 0, gw = 0;
		int flags = 0;
		char if_name[16];

		// Inner loop: iterate all the attributes of one route entry
		struct rtattr *rtap = (struct rtattr *)RTM_RTA(rtp);
		int rtl = RTM_PAYLOAD(nlp);
		for (; RTA_OK(rtap, rtl); rtap = RTA_NEXT(rtap, rtl)) {
			switch(rtap->rta_type) {
				// destination IPv4 address
				case RTA_DST:
					dest = ntohl(*(u32 *)RTA_DATA(rtap));
					mask = 0xFFFFFFFF << (32 - rtp->rtm_dst_len);
					break;
				case RTA_GATEWAY:
					gw = ntohl(*(u32 *)RTA_DATA(rtap));
					break;
				case RTA_PREFSRC:
					// tmp_entry.addr = *(u32 *)RTA_DATA(rtap);
					break;
				case RTA_OIF:
					if_index_to_name(*((int *) RTA_DATA(rtap)), if_name);
					break;
				default:
					break;
			}
		}

		flags |= RTF_UP;
		if (gw != 0)
			flags |= RTF_GATEWAY;
		if (mask == (u32)(-1))
			flags |= RTF_HOST; 

		iface_info_t *iface = if_name_to_iface(if_name);
		if (iface) {
			// the desired interface
			rt_entry_t *entry = new_rt_entry(dest, mask, gw, iface);
			add_rt_entry(entry);

			n += 1;
		}
	}

	return n;
}

void load_rtable_from_kernel()
{
	char buf[ROUTE_BATCH_SIZE];
	int len = get_unparsed_route_info(buf, ROUTE_BATCH_SIZE);
	int n = parse_routing_info(buf, len);

	fprintf(stdout, "Routing table of %d entries has been loaded.\n", n);
}
