#ifndef __ICMP_H__
#define __ICMP_H__

#include "types.h"
#include "checksum.h"
#include "base.h"

struct icmphdr {
	u8	type;
	u8	code;
	u16	checksum;
	union {
		struct {
			u16 identifier;
			u16 sequence;
		} is;
		struct {
			u16 unused;
			u16 mtu;
		} um;
	} u;
#define icmp_identifier u.is.identifier
#define icmp_sequence	u.is.sequence
#define icmp_mtu	u.um.mtu
}__attribute__((packed));

#define ICMP_HDR_SIZE	sizeof(struct icmphdr)
#define ICMP_COPIED_DATA_LEN	8

#define ICMP_ECHOREQUEST		8       /* Echo Request					*/
#define ICMP_ECHOREPLY          0       /* Echo Reply                   */
#define ICMP_DEST_UNREACH       3       /* Destination Unreachable      */
#define ICMP_TIME_EXCEEDED      11      /* Time Exceeded                */

/* Codes for UNREACH. */
#define ICMP_NET_UNREACH        0       /* Network Unreachable          */
#define ICMP_HOST_UNREACH       1       /* Host Unreachable             */
#define ICMP_PROT_UNREACH       2       /* Protocol Unreachable         */
#define ICMP_PORT_UNREACH       3       /* Port Unreachable             */

/* Codes for TIME_EXCEEDED. */
#define ICMP_EXC_TTL            0       /* TTL count exceeded           */

static inline u16 icmp_checksum(struct icmphdr *icmp, int len)
{
	u16 tmp = icmp->checksum;
	icmp->checksum = 0;
	u16 sum = checksum((u16 *)icmp, len, 0);
	icmp->checksum = tmp;

	return sum;
}

void icmp_send_packet(const char *in_pkt, int len, u8 type, u8 code);

#endif
