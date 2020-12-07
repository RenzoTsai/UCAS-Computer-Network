#ifndef __HASH_H__
#define __HASH_H__

#include "types.h"

#define HASH_8BITS 256
#define HASH_16BITS 65536

// the simplest hash functions, you can recreate the wheels as you wish

static inline u8 hash8(char *buf, int len)
{
	u8 result = 0;
	for (int i = 0; i < len; i++)
		result ^= buf[i];

	return result;
}

static inline u16 hash16(char *buf, int len)
{
	u16 result = 0;
	for (int i = 0; i < len / 2 * 2; i += 2)
		result ^= *(u16 *)(buf + i);

	if (len % 2)
		result ^= (u8)(buf[len-1]);
	
	return result;
}


#endif
