#ifndef __CHECKSUM_H__
#define __CHECKSUM_H__

#include "types.h"

// calculate the checksum of the given buf, providing sum 
// as the initial value
static inline u16 checksum(u16 *ptr, int nbytes, u32 sum)
{
	if (nbytes % 2) {
		sum += ((u8 *)ptr)[--nbytes];
	}
 
    while (nbytes > 0) {
        sum += *ptr++;
        nbytes -= 2;
    }
 
    sum = (sum >> 16) + (sum & 0xffff);
    sum = sum + (sum >> 16);

    return (u16)~sum;
}

#endif
