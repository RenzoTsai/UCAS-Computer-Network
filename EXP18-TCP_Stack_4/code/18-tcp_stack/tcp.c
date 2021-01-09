#include "ip.h"
#include "tcp.h"
#include "tcp_sock.h"

#include "log.h"

#include <arpa/inet.h>

const char *tcp_state_str[] = { "CLOSED", "LISTEN", "SYN_RECV",
	"SYN_SENT", "ESTABLISHED", "CLOSE_WAIT", "LAST_ACK", "FIN_WAIT-1",
	"FIN_WAIT-2", "CLOSING", "TIME_WAIT",
};

// assign an initial sending sequence number for a new tcp sock
u32 tcp_new_iss()
{
	return (u32)rand();
}

static int copy_flag_str(u8 flags, int flag, char *buf, int start, 
		const char *str, int len)
{
	if (flags & flag) {
		strcpy(buf + start, str);
		return len;
	}

	return 0;
}

// copy flag string to buf
void tcp_copy_flags_to_str(u8 flags, char buf[32])
{
	int len = 0;
	memset(buf, 0, 32);

	len += copy_flag_str(flags, TCP_FIN, buf, len, "FIN|", 4);
	len += copy_flag_str(flags, TCP_SYN, buf, len, "SYN|", 4);
	len += copy_flag_str(flags, TCP_RST, buf, len, "RST|", 4);
	len += copy_flag_str(flags, TCP_PSH, buf, len, "PSH|", 4);
	len += copy_flag_str(flags, TCP_ACK, buf, len, "ACK|", 4);
	len += copy_flag_str(flags, TCP_URG, buf, len, "URG|", 4);

	if (len != 0)
		buf[len-1] = '\0';
}

// let tcp control block (cb) to store all the necessary information of a TCP
// packet
void tcp_cb_init(struct iphdr *ip, struct tcphdr *tcp, struct tcp_cb *cb)
{
	int len = ntohs(ip->tot_len) - IP_HDR_SIZE(ip) - TCP_HDR_SIZE(tcp);
	cb->saddr = ntohl(ip->saddr);
	cb->daddr = ntohl(ip->daddr);
	cb->sport = ntohs(tcp->sport);
	cb->dport = ntohs(tcp->dport);
	cb->seq = ntohl(tcp->seq);
	cb->seq_end = cb->seq + len + ((tcp->flags & (TCP_SYN|TCP_FIN)) ? 1 : 0);
	cb->ack = ntohl(tcp->ack);
	cb->payload = (char *)tcp + tcp->off * 4;
	cb->pl_len = len;
	cb->rwnd = ntohs(tcp->rwnd);
	cb->flags = tcp->flags;
}

// handle TCP packet: find the appropriate tcp sock, and let the tcp sock 
// to process the packet.
void handle_tcp_packet(char *packet, struct iphdr *ip, struct tcphdr *tcp)
{
	if (tcp_checksum(ip, tcp) != tcp->checksum) {
		log(ERROR, "received tcp packet with invalid checksum, drop it.");
		return ;
	}

	struct tcp_cb cb;
	tcp_cb_init(ip, tcp, &cb);

	struct tcp_sock *tsk = tcp_sock_lookup(&cb);

	tcp_process(tsk, &cb, packet);
}
