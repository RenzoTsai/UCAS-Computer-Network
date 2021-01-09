#ifndef __TCP_HASH_H__
#define __TCP_HASH_H__

#include "hash.h"
#include "list.h"
#include "tcp_sock.h"

#define TCP_HASH_SIZE HASH_8BITS
#define TCP_HASH_MASK (TCP_HASH_SIZE - 1)

// the 3 tables in tcp_hash_table
struct tcp_hash_table {
	struct list_head established_table[TCP_HASH_SIZE];
	struct list_head listen_table[TCP_HASH_SIZE];
	struct list_head bind_table[TCP_HASH_SIZE];
};

// tcp hash function: if hashed into bind_table or listen_table, only use sport; 
// otherwise, use all the 4 arguments
static inline int tcp_hash_function(u32 saddr, u32 daddr, u16 sport, u16 dport)
{
	int result = hash8((char *)&saddr, 4) ^ hash8((char *)&daddr, 4) ^ \
				 hash8((char *)&sport, 2) ^ hash8((char *)&dport, 2);

	return result & TCP_HASH_MASK;
}

#endif
