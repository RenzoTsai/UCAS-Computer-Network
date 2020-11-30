#include "mospf_proto.h"
#include "base.h"
#include <arpa/inet.h>

extern ustack_t *instance;

void mospf_init_hdr(struct mospf_hdr *mospf, u8 type, u16 len, u32 rid, u32 aid)
{
	mospf->version = MOSPF_VERSION;
	mospf->type = type;
	mospf->len = htons(len);
	mospf->rid = htonl(rid);
	mospf->aid = htonl(aid);
	mospf->padding = 0;
}

void mospf_init_hello(struct mospf_hello *hello, u32 mask)
{
	hello->mask = htonl(mask);
	hello->helloint = htons(MOSPF_DEFAULT_HELLOINT);
	hello->padding = 0;
}

void mospf_init_lsu(struct mospf_lsu *lsu, u32 nadv)
{
	lsu->seq = htons(instance->sequence_num);
	lsu->unused = 0;
	lsu->ttl = MOSPF_MAX_LSU_TTL;
	lsu->nadv = htonl(nadv);
}
