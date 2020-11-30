#ifndef __MOSPF_PROTO_H__
#define __MOSPF_PROTO_H__

#include "types.h"
#include "checksum.h"

#include <arpa/inet.h>

#define MOSPF_VERSION 			2	// MOSPF Version

#define MOSPF_ALLSPFRouters 	0xe0000005 // 224.0.0.5

#define MOSPF_TYPE_HELLO		1
#define MOSPF_TYPE_LSU			4

#define MOSPF_DEFAULT_HELLOINT		5	// 5 seconds
#define	MOSPF_DEFAULT_LSUINT		30	// 30 seconds
#define MOSPF_DATABASE_TIMEOUT		40	// 40 seconds

#define MOSPF_MAX_LSU_TTL		255

struct mospf_hdr {
	u8		version;
	u8		type;
	u16		len;
	u32		rid;
	u32		aid;
	u16		checksum;
	u16		padding;
}__attribute__ ((packed));
#define MOSPF_HDR_SIZE sizeof(struct mospf_hdr)

struct mospf_hello {
	u32		mask;		// network mask associated with this interface
	u16		helloint;	// number of seconds between hellos from this router
	u16		padding;	// set to zero
}__attribute__ ((packed));
#define MOSPF_HELLO_SIZE sizeof(struct mospf_hello)

struct mospf_lsu {
	u16		seq;
	u8		ttl;
	u8		unused;
	u32		nadv;
}__attribute__ ((packed));
#define MOSPF_LSU_SIZE sizeof(struct mospf_lsu)

struct mospf_lsa {
	u32		network;
	u32		mask;
	u32		rid;
}__attribute__ ((packed));
#define MOSPF_LSA_SIZE sizeof(struct mospf_lsa)

static inline u16 mospf_checksum(struct mospf_hdr *mospf)
{
	u16 tmp = mospf->checksum;
	mospf->checksum = 0;
	u16 sum = checksum((u16 *)mospf, ntohs(mospf->len), 0);
	mospf->checksum = tmp;

	return sum;
}

void mospf_init_hdr(struct mospf_hdr *mospf, u8 type, u16 len, u32 rid, u32 aid);
void mospf_init_hello(struct mospf_hello *hello, u32 mask);
void mospf_init_lsu(struct mospf_lsu *lsu, u32 nadv);

#endif
