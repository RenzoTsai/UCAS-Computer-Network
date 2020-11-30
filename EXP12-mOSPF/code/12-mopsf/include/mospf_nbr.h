#ifndef __NOSPF_NBR_H__
#define __NOSPF_NBR_H__

#include "base.h"
#include "types.h"
#include "list.h"

typedef struct {
	struct list_head list;
	u32 	nbr_id;			// neighbor ID
	u32		nbr_ip;			// neighbor IP
	u32		nbr_mask;		// neighbor mask
	u8		alive;			// alive for #(seconds)
} mospf_nbr_t;

#endif
