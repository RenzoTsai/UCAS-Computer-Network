#ifndef __ICMP_H__
#define __ICMP_H__

#include "types.h"
#include "checksum.h"
#include "base.h"

struct icmphdr {
	u8	type;				// type of icmp message
	u8	code;				// icmp code
	u16	checksum;			
	u16 icmp_identifier;	// icmp identifier, used in icmp echo request
	u16 icmp_sequence;		// icmp sequence, used in icmp echo request
}__attribute__((packed));

#define ICMP_HDR_SIZE	sizeof(struct icmphdr)
#define ICMP_COPIED_DATA_LEN	8

#define ICMP_ECHOREQUEST		8       // echo request	
#define ICMP_ECHOREPLY          0       // echo reply                   
#define ICMP_DEST_UNREACH       3       // destination unreachable      
#define ICMP_TIME_EXCEEDED      11      // time exceeded                

// codes for UNREACH
#define ICMP_NET_UNREACH        0       // network unreachable          
#define ICMP_HOST_UNREACH       1       // host unreachable             

// code for TIME_EXCEEDED
#define ICMP_EXC_TTL            0       // ttl count exceeded

// calculate the checksum of icmp data, note that the length of icmp data varies
static inline u16 icmp_checksum(struct icmphdr *icmp, int len)
{
	u16 tmp = icmp->checksum;
	icmp->checksum = 0;
	u16 sum = checksum((u16 *)icmp, len, 0);
	icmp->checksum = tmp;

	return sum;
}

// construct icmp packet according to type, code and incoming packet, and send it
void icmp_send_packet(const char *in_pkt, int len, u8 type, u8 code);

#endif
