#include "icmp.h"
#include "ip.h"
#include "rtable.h"
#include "arp.h"
#include "base.h"

#include "log.h"

#include <stdlib.h>

void icmp_send_packet(const char *in_pkt, int len, u8 type, u8 code)
{
	struct iphdr *in_ip = packet_to_ip_hdr(in_pkt);
	struct icmphdr *in_icmp = (struct icmphdr *)(IP_DATA(in_ip));

	int out_len = 0;
	int icmp_len = 0;
	if (type == ICMP_ECHOREPLY) {
		out_len = len;
		icmp_len = ntohs(in_ip->tot_len) - IP_HDR_SIZE(in_ip);
	}
	else {
		icmp_len = ICMP_HDR_SIZE + IP_HDR_SIZE(in_ip) + ICMP_COPIED_DATA_LEN;
		out_len = ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + icmp_len;
	}

	char *out_pkt = malloc(out_len);
	if (!out_pkt) {
		log(ERROR, "malloc packet failed when sending icmp packet.");
		return ;
	}
	memset(out_pkt, 0, out_len);

	struct iphdr *out_ip = packet_to_ip_hdr(out_pkt);
	struct icmphdr *out_icmp = (struct icmphdr *)((char *)out_ip + IP_BASE_HDR_SIZE);
	out_icmp->type = type;
	out_icmp->code = code;

	if (type == ICMP_ECHOREPLY) {
		// ICMP reply is the replica of the ICMP request data.
		out_icmp->icmp_identifier = in_icmp->icmp_identifier;
		out_icmp->icmp_sequence = in_icmp->icmp_sequence;
		memcpy((char *)out_icmp + ICMP_HDR_SIZE, (char *)in_icmp + ICMP_HDR_SIZE, \
				icmp_len - ICMP_HDR_SIZE);
	}
	else {
		// make sure that the incoming ip packet has enough bytes
		memcpy((char *)out_icmp + ICMP_HDR_SIZE, (char *)in_ip, icmp_len - ICMP_HDR_SIZE);
	}

	out_icmp->checksum = icmp_checksum(out_icmp, icmp_len);

	u32 saddr = 0, daddr = ntohl(in_ip->saddr);
	if (type == ICMP_ECHOREPLY) {
		saddr = ntohl(in_ip->daddr);
	}
	else {
		// ICMP_DEST_UNREACH or ICMP_TIME_EXCEEDED
		rt_entry_t *entry = longest_prefix_match(daddr);
		if (!entry) {
			log(ERROR, "could not find route entry when sending icmp packet, impossible.");
			free(out_pkt);
			return ;
		}
		saddr = entry->iface->ip;
	}

	int out_ip_len = IP_BASE_HDR_SIZE + icmp_len;

	ip_init_hdr(out_ip, saddr, daddr, out_ip_len, IPPROTO_ICMP);

	ip_send_packet(out_pkt, out_len);
}
