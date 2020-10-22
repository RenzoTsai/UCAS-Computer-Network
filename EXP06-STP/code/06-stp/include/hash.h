#ifndef __HASH_H__
#define __HASH_H__

#include "types.h"

#define HASH_8BITS 256
#define HASH_16BITS 65536


u8 hash8(unsigned char *addr, int len);

u16 hash16(unsigned char *addr, int len);


#endif
