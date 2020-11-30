#ifndef __PACKET_H__
#define __PACKET_H__

#include "base.h"
#include "types.h"

void iface_send_packet(iface_info_t *iface, char *packet, int len);
void broadcast_packet(iface_info_t *iface, char *packet, int len);

#endif
