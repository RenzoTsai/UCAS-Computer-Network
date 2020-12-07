#ifndef __TCP_H__
#define __TCP_H__

#include "types.h"
#include "ip.h"
#include "checksum.h"

#include <endian.h>

// format of standard tcp header 
struct tcphdr {
	u16 sport;		// source port 
	u16 dport;		// destination port
	u32 seq;			// sequence number
	u32 ack;			// acknowledgement number
# if __BYTE_ORDER == __LITTLE_ENDIAN
	u8 x2:4;			// (unused)
	u8 off:4;			// data offset
# elif __BYTE_ORDER == __BIG_ENDIAN
	u8 off:4;			// data offset
	u8 x2:4;			// (unused)
# endif
	u8 flags;
# define TCP_FIN	0x01
# define TCP_SYN	0x02
# define TCP_RST	0x04
# define TCP_PSH	0x08
# define TCP_ACK	0x10
# define TCP_URG	0x20
	u16 rwnd;			// receiving window
	u16 checksum;		// checksum
	u16 urp;			// urgent pointer
} __attribute__((packed));

#define TCP_HDR_OFFSET 5
#define TCP_BASE_HDR_SIZE 20
#define TCP_HDR_SIZE(tcp) (tcp->off * 4)

#define TCP_DEFAULT_WINDOW 65535

// control block, representing all the necesary information of a packet
struct tcp_cb {
	u32 saddr;		// source addr of the packet
	u32 daddr;		// source port of the packet
	u16 sport;		// dest addr of the packet
	u16 dport;		// dest port of the packet
	u32 seq;		// sequence number in tcp header
	u32 seq_end;		// seq + (SYN|FIN) + len(payload)
	u32 ack;		// ack number in tcp header
	char *payload;		// pointer to tcp data
	int pl_len;		// the length of tcp data
	u32 rwnd;		// receiving window in tcp header
	u8 flags;		// flags in tcp header
	struct iphdr *ip;		// pointer to ip header
	struct tcphdr *tcp;		// pointer to tcp header
};

// tcp states
enum tcp_state { TCP_CLOSED, TCP_LISTEN, TCP_SYN_RECV, TCP_SYN_SENT, \
	TCP_ESTABLISHED, TCP_CLOSE_WAIT, TCP_LAST_ACK, TCP_FIN_WAIT_1, \
	TCP_FIN_WAIT_2, TCP_CLOSING, TCP_TIME_WAIT };

static inline struct tcphdr *packet_to_tcp_hdr(char *packet)
{
	struct iphdr *ip = packet_to_ip_hdr(packet);
	return (struct tcphdr *)((char *)ip + IP_HDR_SIZE(ip));
}

static inline u16 tcp_checksum(struct iphdr *ip, struct tcphdr *tcp)
{
	u16 tmp = tcp->checksum;
	tcp->checksum = 0;

	u16 reserv_proto = ip->protocol;
	u16 tcp_len = ntohs(ip->tot_len) - IP_HDR_SIZE(ip);

	u32 sum = ip->saddr + ip->daddr + htons(reserv_proto) + htons(tcp_len);
	u16 cksum = checksum((u16 *)tcp, (int)tcp_len, sum);

	tcp->checksum = tmp;

	return cksum;
}

static inline u32 tcp_seq_end(struct iphdr *ip, struct tcphdr *tcp)
{
	int len = ntohs(ip->tot_len) - IP_HDR_SIZE(ip) - TCP_HDR_SIZE(tcp);
	return ntohl(tcp->seq) + len + ((tcp->flags & (TCP_SYN|TCP_FIN)) ? 1 : 0);
}

extern const char *tcp_state_str[];
static inline const char *tcp_state_to_str(int state)
{
	return tcp_state_str[state];
}

void tcp_copy_flags_to_str(u8 flags, char buf[]);
void tcp_cb_init(struct iphdr *ip, struct tcphdr *tcp, struct tcp_cb *cb);
void handle_tcp_packet(char *packet, struct iphdr *ip, struct tcphdr *tcp);

#endif
