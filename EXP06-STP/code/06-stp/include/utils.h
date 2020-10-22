#ifndef __UTILS_H__
#define __UTILS_H__

#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define htonll(x) ((1==htonl(1)) ? (x) : ((u64)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) htonll(x)

#define exit_if_null(ptr) \
	do { \
		if (!(ptr)) { \
			fprintf(stderr, #ptr " should not be NULL in " __FILE__ ":%d", __LINE__); \
			exit(1); \
		} \
	} while(0) 

static inline void *safe_malloc(int size) 
{
	void *ptr = malloc(size);
	exit_if_null(ptr);
	return ptr;
}

#endif
