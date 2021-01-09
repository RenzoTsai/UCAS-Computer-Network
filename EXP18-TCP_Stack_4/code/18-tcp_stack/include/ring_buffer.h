#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct ring_buffer {
	int size;
	int head;		// read from head
	int tail;		// write from tail
	char buf[0];
};

static inline struct ring_buffer *alloc_ring_buffer(int size)
{
	// there is always one byte which should not be read or written
	int tot_size = sizeof(struct ring_buffer) + size + 1;
	struct ring_buffer *rbuf = malloc(tot_size);
	memset(rbuf, 0, tot_size);
	rbuf->size = size + 1;

	return rbuf;
}

static inline void free_ring_buffer(struct ring_buffer *rbuf)
{
	free(rbuf);
}

static inline int ring_buffer_used(struct ring_buffer *rbuf)
{
	return (rbuf->tail - rbuf->head + rbuf->size) % (rbuf->size);
}

static inline int ring_buffer_free(struct ring_buffer *rbuf)
{
	// let 1 byte to distinguish empty buffer and full buffer
	return rbuf->size - ring_buffer_used(rbuf) - 1;
}

static inline int ring_buffer_empty(struct ring_buffer *rbuf)
{
	return ring_buffer_used(rbuf) == 0;
}

static inline int ring_buffer_full(struct ring_buffer *rbuf)
{
	return ring_buffer_free(rbuf) == 0;
}

#ifndef min
#define min(x,y) ((x)<(y) ? (x) : (y))
#endif

static inline int read_ring_buffer(struct ring_buffer *rbuf, char *buf, int size)
{
	int len = min(ring_buffer_used(rbuf), size);
	if (len > 0) {
		if (rbuf->head + len > rbuf->size) {
			int right = rbuf->size - rbuf->head,
				left = len - right;
			memcpy(buf, rbuf->buf + rbuf->head, right);
			memcpy(buf + right, rbuf->buf, left);
		}
		else {
			memcpy(buf, rbuf->buf + rbuf->head, len);
		}

		rbuf->head = (rbuf->head + len) % (rbuf->size);
	}

	return len;
}

// rbuf should have enough space for buf
static inline void write_ring_buffer(struct ring_buffer *rbuf, char *buf, int size)
{
	assert(size > 0 && ring_buffer_free(rbuf) >= size);
	int len = size;
	if (rbuf->tail + len > rbuf->size) {
		int right = rbuf->size - rbuf->tail,
			left = len - right;
		memcpy(rbuf->buf + rbuf->tail, buf, right);
		memcpy(rbuf->buf, buf + right, left);
	}
	else {
		memcpy(rbuf->buf + rbuf->tail, buf, len);
	}

	rbuf->tail = (rbuf->tail + len) % (rbuf->size);
}

#endif
