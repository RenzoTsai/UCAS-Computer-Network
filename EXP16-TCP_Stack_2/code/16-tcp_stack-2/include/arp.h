#ifndef __ARP_H__
#define __ARP_H__

#include "base.h"
#include "ether.h"
#include "types.h"

#define ARPHRD_ETHER 1

#define ARPOP_REQUEST 1
#define ARPOP_REPLY 2

struct ether_arp {
    u16 arp_hrd;		// format of hardware address, should be 0x01
    u16 arp_pro;		// format of protocol address, should be 0x0800
    u8	arp_hln;		// length of hardware address, should be 6
    u8	arp_pln;		// length of protocol address, should be 4
    u16 arp_op;			// ARP opcode (command)
	u8	arp_sha[ETH_ALEN];	// sender hardware address
	u32	arp_spa;		// sender protocol address
	u8	arp_tha[ETH_ALEN];	// target hardware address
	u32	arp_tpa;		// target protocol address
} __attribute__ ((packed));

// get the arp header of the packet
static inline struct ether_arp *packet_to_ether_arp(const char *packet)
{
	return (struct ether_arp *)(packet + ETHER_HDR_SIZE);
}

void handle_arp_packet(iface_info_t *info, char *packet, int len);
void arp_send_request(iface_info_t *iface, u32 dst_ip);
void iface_send_packet_by_arp(iface_info_t *iface, u32 dst_ip, char *packet, int len);

#endif
