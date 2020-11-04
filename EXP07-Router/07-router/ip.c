#include "ip.h"

#include <stdio.h>
#include <stdlib.h>
#include "icmp.h"
#include "rtable.h"

void ip_forward_package(iface_info_t *iface, char *packet, int len); 

// handle ip packet
//
// If the packet is ICMP echo request and the destination IP address is equal to
// the IP address of the iface, send ICMP echo reply; otherwise, forward the
// packet.
void handle_ip_packet(iface_info_t *iface, char *packet, int len)
{
	fprintf(stderr, "TODO: handle ip packet.\n");
	struct iphdr * ip_header = packet_to_ip_hdr(packet);
	struct icmphdr * icmp_header = (struct icmphdr *)(IP_DATA(ip_header));
	u32 dst_addr = ntohl(ip_header->daddr);
	
	if (icmp_header->type == ICMP_ECHOREQUEST && iface->ip == dst_addr) {
		icmp_send_packet(packet, len, ICMP_ECHOREPLY, 0);
	} else {
		ip_forward_package(iface, packet, len);
	}
}

 void ip_forward_package(iface_info_t *iface, char *packet, int len){
	fprintf(stderr, "TODO: forward ip packet.\n");
	struct iphdr * ip_header = packet_to_ip_hdr(packet);

	ip_header->ttl--;
	if (ip_header->ttl <= 0) {
		icmp_send_packet(packet, len, ICMP_TIME_EXCEEDED, ICMP_EXC_TTL);
		return;
	}

	u32 dst_addr = ntohl(ip_header->daddr);
	ip_header->checksum = ip_checksum(ip_header);
	// struct ether_header *eh = (struct ether_header *)packet;


	rt_entry_t *rt_entry = NULL;
	// rt_entry = longest_prefix_match(dst_addr);
	// memcpy(eh->ether_shost, iface->mac, ETH_ALEN);
	// memcpy(eh->ether_dhost, rt_entry->iface->mac, ETH_ALEN);
	if ((rt_entry = longest_prefix_match(dst_addr)) == NULL) {
		icmp_send_packet(packet, len, ICMP_DEST_UNREACH, ICMP_NET_UNREACH);
	} else {
		ip_send_packet(packet, len);
	}
}
