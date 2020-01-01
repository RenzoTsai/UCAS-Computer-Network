#include "headers.h"
#include "base.h"
#include "ether.h"
#include "log.h"

#include <sys/types.h>
#include <ifaddrs.h>

ustack_t *instance;

static iface_info_t *fd_to_iface(int fd)
{
	iface_info_t *iface = NULL;
	list_for_each_entry(iface, &instance->iface_list, list) {
		if (iface->fd == fd)
			return iface;
	}

	log(ERROR, "Could not find the desired interface according to fd %d", fd);

	return NULL;
}

void iface_send_packet(iface_info_t *iface, const char *packet, int len)
{
	struct sockaddr_ll addr;
	memset(&addr, 0, sizeof(struct sockaddr_ll));
	addr.sll_family = AF_PACKET;
	addr.sll_ifindex = iface->index;
	addr.sll_halen = ETH_ALEN;
	addr.sll_protocol = htons(ETH_P_ARP);
	struct ether_header *eh = (struct ether_header *)packet;
	memcpy(addr.sll_addr, eh->ether_dhost, ETH_ALEN);

	if (sendto(iface->fd, packet, len, 0, (const struct sockaddr *)&addr,
				sizeof(struct sockaddr_ll)) < 0) {
		perror("Send raw packet failed");
	}
}

void broadcast_packet(iface_info_t *iface, const char *packet, int len)
{
	// TODO: broadcast packet 
	fprintf(stdout, "TODO: broadcast packet here.\n");
}

void handle_packet(iface_info_t *iface, char *packet, int len)
{
	broadcast_packet(iface, packet, len);
	free(packet);
}

int open_device(const char *dname)
{
	int sd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (sd < 0) { 
		perror("creating SOCK_RAW failed!");
		return -1;
	}

	struct ifreq ifr;
	bzero(&ifr, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, dname);
	if (ioctl(sd, SIOCGIFINDEX, &ifr) < 0) {
		perror("ioctl() SIOCGIFINDEX failed!");
		return -1;
	}

	struct sockaddr_ll sll;
	bzero(&sll, sizeof(sll));
	sll.sll_family = AF_PACKET;
	sll.sll_ifindex = ifr.ifr_ifindex;

	if (bind((int)sd, (struct sockaddr *) &sll, sizeof(sll)) < 0) {
		perror("binding to device failed!");
		return -1;
	}

	if (ioctl(sd, SIOCGIFHWADDR, &ifr) < 0) {
		perror("Start(): SIOCGIFHWADDR failed!");
		return -1;
	}

	// We could still capture all the packets even if not in promisc mode.
#if 0
	struct packet_mreq mr;
	bzero(&mr, sizeof(mr));
	mr.mr_ifindex = sll.sll_ifindex;
	mr.mr_type = PACKET_MR_PROMISC;

	if (setsockopt(sd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) < 0) {
		perror("Start(): setsockopt() PACKET_ADD_MEMBERSHIP failed!");
		return -1;
	}
#endif
	
	return sd;
}

int read_iface_info(iface_info_t *iface)
{
	int fd = open_device(iface->name);

	iface->fd = fd;

	int s = socket(AF_INET, SOCK_DGRAM, 0);
	struct ifreq ifr;
	strcpy(ifr.ifr_name, iface->name);

	ioctl(s, SIOCGIFINDEX, &ifr);
	iface->index = ifr.ifr_ifindex;

	ioctl(s, SIOCGIFHWADDR, &ifr);
	memcpy(&iface->mac, ifr.ifr_hwaddr.sa_data, sizeof(iface->mac));

	// As a broadcast (hub), its interfaces have no IP address.
#if 0
	struct in_addr ip, mask;
	// get ip address
	if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
		perror("Get IP address failed");
		exit(1);
	}
	ip = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
	iface->ip = ntohl(*(u32 *)&ip);
	strcpy(iface->ip_str, inet_ntoa(ip));

	// get net mask 
	if (ioctl(fd, SIOCGIFNETMASK, &ifr) < 0) {
		perror("Get IP mask failed");
		exit(1);
	}
	mask = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
	iface->mask = ntohl(*(u32 *)&mask);
#endif

	return fd;
}

static void find_available_ifaces()
{
	init_list_head(&instance->iface_list);

	struct ifaddrs *addrs,*addr;
	getifaddrs(&addrs);
	for (addr = addrs; addr != NULL; addr = addr->ifa_next) {
		if (addr->ifa_addr && addr->ifa_addr->sa_family == AF_PACKET && \
				strstr(addr->ifa_name, "-eth") != NULL) {

			iface_info_t *iface = malloc(sizeof(iface_info_t));
			bzero(iface, sizeof(iface_info_t));

			init_list_head(&iface->list);
			strcpy(iface->name, addr->ifa_name);

			list_add_tail(&iface->list, &instance->iface_list);

			instance->nifs += 1;
		}
	}
	freeifaddrs(addrs);

	if (instance->nifs == 0) {
		log(ERROR, "could not find available interfaces.");
		exit(1);
	}

	char dev_names[1024] = "";
	iface_info_t *iface = NULL;
	list_for_each_entry(iface, &instance->iface_list, list) {
		sprintf(dev_names + strlen(dev_names), " %s", iface->name);
	}
	log(DEBUG, "find the following interfaces: %s.", dev_names);
}

void init_all_ifaces()
{
	find_available_ifaces();

	instance->fds = malloc(sizeof(struct pollfd) * instance->nifs);
	bzero(instance->fds, sizeof(struct pollfd) * instance->nifs);

	iface_info_t *iface = NULL;
	int i = 0;
	list_for_each_entry(iface, &instance->iface_list, list) {
		int fd = read_iface_info(iface);
		instance->fds[i].fd = fd;
		instance->fds[i].events |= POLLIN;

		i += 1;
	}
}

void init_ustack()
{
	instance = malloc(sizeof(ustack_t));

	bzero(instance, sizeof(ustack_t));
	init_list_head(&instance->iface_list);

	init_all_ifaces();
}

void ustack_run()
{
	struct sockaddr_ll addr;
	socklen_t addr_len = sizeof(addr);
	char buf[ETH_FRAME_LEN];
	int len;

	while (1) {
		int ready = poll(instance->fds, instance->nifs, -1);
		if (ready < 0) {
			perror("Poll failed!");
			break;
		}
		else if (ready == 0)
			continue;

		for (int i = 0; i < instance->nifs; i++) {
			if (instance->fds[i].revents & POLLIN) {
				len = recvfrom(instance->fds[i].fd, buf, ETH_FRAME_LEN, 0, \
						(struct sockaddr*)&addr, &addr_len);
				if (len <= 0) {
					log(ERROR, "receive packet error: %s", strerror(errno));
				}
				else if (addr.sll_pkttype == PACKET_OUTGOING) {
					// XXX: Linux raw socket will capture both incoming and
					// outgoing packets, while we only care about the incoming ones.

					// log(DEBUG, "received packet which is sent from the "
					// 		"interface itself, drop it.");
				}
				else {
					iface_info_t *iface = fd_to_iface(instance->fds[i].fd);
					if (!iface) 
						continue;

					char *packet = malloc(len);
					if (!packet) {
						log(ERROR, "malloc failed when receiving packet.");
						continue;
					}
					memcpy(packet, buf, len);
					handle_packet(iface, packet, len);
				}
			}
		}
	}
}

int main(int argc, const char **argv)
{
	if (getuid() && geteuid()) {
		printf("Permission denied, should be superuser!\n");
		exit(1);
	}

	init_ustack();

	ustack_run();

	return 0;
}
