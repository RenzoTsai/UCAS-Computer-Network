#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include <stdlib.h>

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
