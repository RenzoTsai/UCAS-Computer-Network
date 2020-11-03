#include "icmp.h"
#include "ip.h"
#include "rtable.h"
#include "arp.h"
#include "base.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// send icmp packet
void icmp_send_packet(const char *in_pkt, int len, u8 type, u8 code)
{
	// fprintf(stderr, "TODO: malloc and send icmp packet.\n");
	
	int packet_len = 0; 
	struct iphdr *in_ip_hdr = packet_to_ip_hdr(in_pkt);

	if (type == ICMP_ECHOREPLY) {
		packet_len = len;
	} else {
		packet_len = ETHER_HDR_SIZE + ICMP_HDR_SIZE + IP_HDR_SIZE(in_ip_hdr) + 8;
	}
	
	char *packet = (char *)malloc(packet_len);

	struct ether_header *eh = (struct ether_header *)packet;
    eh->ether_type = htons(ETH_P_IP);

	struct iphdr *out_ip_hdr = packet_to_ip_hdr(packet);

	rt_entry_t *rt_entry = longest_prefix_match(ntohl(in_ip_hdr->saddr));

    ip_init_hdr(out_ip_hdr,
                rt_entry->iface->ip,
                ntohl(in_ip_hdr->saddr),
                packet_len - ETHER_HDR_SIZE,
                IPPROTO_ICMP);

	struct icmphdr * icmp_hdr = (struct icmphdr *)(packet + ETHER_HDR_SIZE + IP_HDR_SIZE(in_ip_hdr));
	icmp_hdr->type = type;
	icmp_hdr->code = code;
	
	
	if (type != ICMP_ECHOREPLY) {
		memset(icmp_hdr + ICMP_HDR_SIZE - 4, 0, 4);
		memcpy(icmp_hdr + ICMP_HDR_SIZE, in_ip_hdr, IP_HDR_SIZE(in_ip_hdr) + 8);
	} else {
		memcpy(icmp_hdr + ICMP_HDR_SIZE - 4,
		in_ip_hdr + IP_HDR_SIZE(in_ip_hdr) + 4,
		len - ETHER_HDR_SIZE - IP_HDR_SIZE(in_ip_hdr) - 4);
	}
	icmp_hdr->checksum = icmp_checksum(icmp_hdr, packet_len-ETHER_HDR_SIZE-IP_HDR_SIZE(in_ip_hdr));
	ip_send_packet(packet, packet_len);
}
