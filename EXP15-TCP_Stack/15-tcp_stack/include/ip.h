#ifndef __IP_H__
#define __IP_H__

#include <endian.h>

#include "base.h"
#include "types.h"
#include "checksum.h"
#include "ether.h"

// protocol type specified in ip header 
#ifndef IPPROTO_ICMP
#	define IPPROTO_ICMP		1	// ICMP (Internet Control Message Protocol)
#endif

#ifndef IPPROTO_TCP
#	define IPPROTO_TCP		6	// TCP (Transport Control Protocol)
#endif

#ifndef IPPROTO_UDP
#	define IPPROTO_UDP		17	// UDP (User Datagram Protocol)
#endif

#define IPPROTO_OSPFv2		89 // Open Shortest Path First v2

struct iphdr {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned int ihl:4;					// length of ip header
    unsigned int version:4;				// ip version 
#elif __BYTE_ORDER == __BIG_ENDIAN
    unsigned int version:4;				// ip version
    unsigned int ihl:4;					// length of ip header
#endif
    u8 tos;								// type of service (usually set to 0)
    u16 tot_len;						// total length of ip data
    u16 id;								// ip identifier
    u16 frag_off;						// the offset of ip fragment
    u8 ttl;								// ttl of ip packet
    u8 protocol;						// upper layer protocol, e.g. icmp, tcp, udp 
    u16 checksum;						// checksum of ip header
    u32 saddr;							// source ip address
    u32 daddr;							// destination ip address
};

#define DEFAULT_TTL 64		// default TTL value in ip header
#define IP_DF	0x4000		// do not fragment

#define IP_BASE_HDR_SIZE sizeof(struct iphdr)
#define IP_HDR_SIZE(hdr) (hdr->ihl * 4)
#define IP_DATA(hdr)	((char *)hdr + IP_HDR_SIZE(hdr))

// lazy way to print ip address 'ip' (a u32 variable) as follows:
// fprintf(stdout, "the ip address is "IP_FMT".\n", HOST_IP_FMT_STR(ip));

#define IP_FMT	"%hhu.%hhu.%hhu.%hhu"
#define LE_IP_FMT_STR(ip) ((u8 *)&(ip))[3], \
						  ((u8 *)&(ip))[2], \
 						  ((u8 *)&(ip))[1], \
					      ((u8 *)&(ip))[0]

#define BE_IP_FMT_STR(ip) ((u8 *)&(ip))[0], \
						  ((u8 *)&(ip))[1], \
 						  ((u8 *)&(ip))[2], \
					      ((u8 *)&(ip))[3]

#define NET_IP_FMT_STR(ip)	BE_IP_FMT_STR(ip)

#if __BYTE_ORDER == __LITTLE_ENDIAN
#	define HOST_IP_FMT_STR(ip)	LE_IP_FMT_STR(ip)
#elif __BYTE_ORDER == __BIG_ENDIAN
#	define HOST_IP_FMT_STR(ip)	BE_IP_FMT_STR(ip)
#endif

static inline u16 ip_checksum(struct iphdr *hdr)
{
	u16 tmp = hdr->checksum;
	hdr->checksum = 0;
	u16 sum = checksum((u16 *)hdr, hdr->ihl * 4, 0);
	hdr->checksum = tmp;

	return sum;
}

static inline struct iphdr *packet_to_ip_hdr(const char *packet)
{
	return (struct iphdr *)(packet + ETHER_HDR_SIZE);
}

void ip_init_hdr(struct iphdr *ip, u32 saddr, u32 daddr, u16 len, u8 proto);
void handle_ip_packet(iface_info_t *iface, char *packet, int len);
void ip_send_packet(char *packet, int len);

#endif
