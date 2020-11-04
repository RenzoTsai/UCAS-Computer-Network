#include "arp.h"
#include "base.h"
#include "types.h"
#include "packet.h"
#include "ether.h"
#include "arpcache.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

// send an arp request: encapsulate an arp request packet, send it out through
// iface_send_packet
void arp_send_request(iface_info_t *iface, u32 dst_ip)
{
	fprintf(stderr, "TODO: send arp request when lookup failed in arpcache.\n");
	char * packet = (char *) malloc(ETHER_HDR_SIZE + sizeof(struct ether_arp));

	struct ether_header * ether_hdr = (struct ether_header *)packet;
	struct ether_arp * ether_arp_pkt = (struct ether_arp *)(packet + ETHER_HDR_SIZE);

	ether_hdr->ether_type = htons(ETH_P_ARP);
	memcpy(ether_hdr->ether_shost, iface->mac, ETH_ALEN);
	memset(ether_hdr->ether_dhost, (u8)0xff, ETH_ALEN);

	ether_arp_pkt->arp_hrd = htons(ARPHRD_ETHER);
	ether_arp_pkt->arp_pro = htons(ETH_P_IP);
	ether_arp_pkt->arp_hln = (u8)ETH_ALEN;
	ether_arp_pkt->arp_pln = (u8)4;
	ether_arp_pkt->arp_op = htons(ARPOP_REQUEST);
	memcpy(ether_arp_pkt->arp_sha, iface->mac, ETH_ALEN);
	ether_arp_pkt->arp_spa = htonl(iface->ip);
	memset(ether_arp_pkt->arp_tha, 0, ETH_ALEN);
	ether_arp_pkt->arp_tpa = htonl(dst_ip);
	iface_send_packet(iface, packet, ETHER_HDR_SIZE + sizeof(struct ether_arp));
}

// send an arp reply packet: encapsulate an arp reply packet, send it out
// through iface_send_packet
void arp_send_reply(iface_info_t *iface, struct ether_arp *req_hdr)
{
	// fprintf(stderr, "TODO: send arp reply when receiving arp request.\n");
	char * packet = (char *) malloc(ETHER_HDR_SIZE + sizeof(struct ether_arp));

	struct ether_header * ether_hdr = (struct ether_header *)packet;
	struct ether_arp * ether_arp_pkt = (struct ether_arp *)(packet + ETHER_HDR_SIZE);

	ether_hdr->ether_type = htons(ETH_P_ARP);
	memcpy(ether_hdr->ether_shost, iface->mac, ETH_ALEN);

	memcpy(ether_hdr->ether_dhost, req_hdr->arp_sha, ETH_ALEN);

	ether_arp_pkt->arp_hrd = htons(ARPHRD_ETHER);
	ether_arp_pkt->arp_pro = htons(ETH_P_IP);
	ether_arp_pkt->arp_hln = (u8)ETH_ALEN;
	ether_arp_pkt->arp_pln = (u8)4;
	ether_arp_pkt->arp_op = htons(ARPOP_REPLY);
	memcpy(ether_arp_pkt->arp_sha, iface->mac, ETH_ALEN);
	ether_arp_pkt->arp_spa = htonl(iface->ip);
	memcpy(ether_arp_pkt->arp_tha, req_hdr->arp_sha, ETH_ALEN);
	ether_arp_pkt->arp_tpa = req_hdr->arp_spa;
	iface_send_packet(iface, packet, ETHER_HDR_SIZE + sizeof(struct ether_arp));
}

void handle_arp_packet(iface_info_t *iface, char *packet, int len)
{
	fprintf(stderr, "TODO: process arp packet: arp request & arp reply.\n");
	struct ether_arp * ether_arp_pkt = (struct ether_arp *)(packet + ETHER_HDR_SIZE);
	if (ntohl(ether_arp_pkt->arp_tpa) == iface->ip) {
		if (ntohs(ether_arp_pkt->arp_op) == ARPOP_REPLY) {
			arpcache_insert(ntohl(ether_arp_pkt->arp_spa), ether_arp_pkt->arp_sha);
		} else if (ntohs(ether_arp_pkt->arp_op) == ARPOP_REQUEST) {
			arp_send_reply(iface, ether_arp_pkt);
		}
	}
}

// send (IP) packet through arpcache lookup 
//
// Lookup the mac address of dst_ip in arpcache. If it is found, fill the
// ethernet header and emit the packet by iface_send_packet, otherwise, pending 
// this packet into arpcache, and send arp request.
void iface_send_packet_by_arp(iface_info_t *iface, u32 dst_ip, char *packet, int len)
{
	struct ether_header *eh = (struct ether_header *)packet;
	memcpy(eh->ether_shost, iface->mac, ETH_ALEN);
	eh->ether_type = htons(ETH_P_IP);

	u8 dst_mac[ETH_ALEN];
	int found = arpcache_lookup(dst_ip, dst_mac);
	if (found) {
		log(DEBUG, "found the mac of %x, send this packet", dst_ip);
		memcpy(eh->ether_dhost, dst_mac, ETH_ALEN);
		iface_send_packet(iface, packet, len);
	}
	else {
		log(DEBUG, "lookup %x failed, pend this packet", dst_ip);
		arpcache_append_packet(iface, dst_ip, packet, len);
	}
}
