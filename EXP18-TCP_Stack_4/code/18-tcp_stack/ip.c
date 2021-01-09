#include "ip.h"
#include "icmp.h"
#include "packet.h"
#include "arpcache.h"
#include "rtable.h"
#include "arp.h"
#include "tcp.h"

#include "log.h"

#include <stdlib.h>

void ip_init_hdr(struct iphdr *ip, u32 saddr, u32 daddr, u16 len, u8 proto)
{
	ip->version = 4;
	ip->ihl = 5;
	ip->tos = 0;
	ip->tot_len = htons(len);
	ip->id = rand();
	ip->frag_off = htons(IP_DF);
	ip->ttl = DEFAULT_TTL;
	ip->protocol = proto;
	ip->saddr = htonl(saddr);
	ip->daddr = htonl(daddr);
	ip->checksum = ip_checksum(ip);
}

// the input address is in host byte order
rt_entry_t *longest_prefix_match(u32 dst)
{
	rt_entry_t *entry = NULL, *selected = NULL;

	list_for_each_entry(entry, &rtable, list) {
		if ((dst & entry->mask) == (entry->dest & entry->mask)) {
			if (!selected || selected->mask < entry->mask)
				selected = entry;
		}
	}

	return selected;
}

void ip_forward_packet(u32 ip_dst, char *packet, int len)
{
	struct iphdr *hdr = packet_to_ip_hdr(packet);
	if (hdr->ttl <= 1) {
		icmp_send_packet(packet, len, ICMP_TIME_EXCEEDED, ICMP_EXC_TTL);
		free(packet);
		return ;
	}

	hdr->ttl -= 1;
	hdr->checksum = ip_checksum(hdr);
	rt_entry_t *entry = longest_prefix_match(ip_dst);
	if (!entry) {
		icmp_send_packet(packet, len, ICMP_DEST_UNREACH, ICMP_NET_UNREACH);
		free(packet);
		return ;
	}

	u32 next_hop = get_next_hop(entry, ip_dst);

	// lookup the arpcache.
	u8 dst[ETH_ALEN];
	int found = arpcache_lookup(next_hop, dst);
	if (found) {
		struct ether_header *eh = (struct ether_header *)packet;
		memcpy(eh->ether_dhost, dst, ETH_ALEN);
		memcpy(eh->ether_shost, entry->iface->mac, ETH_ALEN);
		iface_send_packet(entry->iface, packet, len);
	}
	else {
		arpcache_append_packet(entry->iface, next_hop, packet, len);
	}
}

void handle_ip_packet(iface_info_t *iface, char *packet, int len)
{
	struct iphdr *ip = packet_to_ip_hdr(packet);
	u32 daddr = ntohl(ip->daddr);
	if (daddr == iface->ip) {
		if (ip->protocol == IPPROTO_ICMP) {
			struct icmphdr *icmp = (struct icmphdr *)IP_DATA(ip);
			if (icmp->type == ICMP_ECHOREQUEST) {
				icmp_send_packet(packet, len, ICMP_ECHOREPLY, 0);
			}
		}
		else if (ip->protocol == IPPROTO_TCP) {
			handle_tcp_packet(packet, ip, (struct tcphdr *)(IP_DATA(ip)));
		}
		else {
			log(ERROR, "unsupported IP protocol (0x%x) packet.", ip->protocol);
		}

		free(packet);
	}
	else {
		ip_forward_packet(daddr, packet, len);
	}
}

void ip_send_packet(char *packet, int len)
{
	struct iphdr *ip = packet_to_ip_hdr(packet);
	u32 dst = ntohl(ip->daddr);
	rt_entry_t *entry = longest_prefix_match(dst);
	if (!entry) {
		log(ERROR, "Could not find forwarding rule for IP (dst:"IP_FMT") packet.", 
				HOST_IP_FMT_STR(dst));
		free(packet);
		return ;
	}

	u32 next_hop = get_next_hop(entry, dst);
	iface_info_t *iface = entry->iface;

	struct ether_header *eh = (struct ether_header *)packet;
	eh->ether_type = ntohs(ETH_P_IP);
	memcpy(eh->ether_shost, iface->mac, ETH_ALEN);

	iface_send_packet_by_arp(iface, next_hop, packet, len);
}
