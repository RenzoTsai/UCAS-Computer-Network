#ifndef __HASH_H__
#define __HASH_H__

#include "types.h"

#define HASH_8BITS 256
#define HASH_16BITS 65536

static inline u8 hash8(char *addr, int len)
{
	u8 result = 0;
	while (len >= 0) {
		result ^= addr[--len];
	}

	return result;
}

static inline u16 hash16(char *addr, int len)
{
	u16 result = 0;
	while (len >= 2) {
		result ^= *(u8 *)(addr + len - 2);
		len -= 2;
	}

	if (len) 
		result ^= (u8)(*addr);
	
	return result;
}


#endif
