#ifndef __CHECKSUM_H__
#define __CHECKSUM_H__

#include "types.h"

// calculate the checksum of the given buf, providing sum 
// as the initial value
static inline u16 checksum(u16 *buf, int nbytes, u32 sum)
{
	for (int i = 0; i < nbytes / 2; i++)
		sum += buf[i];
 
    sum = (sum >> 16) + (sum & 0xffff);
    sum = sum + (sum >> 16);

	if (nbytes % 2)
		sum += ((u8 *)buf)[nbytes-1];

    return (u16)~sum;
}

#endif
