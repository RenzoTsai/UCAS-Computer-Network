#ifndef __STP_PROTO_H__
#define __STP_PROTO_H__

#include "types.h"

// default priority: switch and port priorities 
#define STP_BRIDGE_PRIORITY 32768
#define STP_PORT_PRIORITY 128

// default timer: one timer tick is 1 / 256 second
#define STP_MAX_AGE    (20000 * 256 / 1000)	// 20 seconds
#define STP_HELLO_TIME (2000 * 256 / 1000)	// 2 seconds
#define STP_FWD_DELAY  (15000 * 256 / 1000) // 15 seconds

// LLC header format
#define LLC_DSAP_SNAP 0x42
#define LLC_SSAP_SNAP 0x42
#define LLC_CNTL_SNAP 3

#define LLC_HDR_SIZE 3
struct llc_header {
	u8 llc_dsap;
	u8 llc_ssap;
	u8 llc_cntl;
} __attribute__((packed));

#define STP_PROTOCOL_ID 0x0000
#define STP_PROTOCOL_VERSION 0x00
#define STP_TYPE_CONFIG 0x00		// indicating STP config packet
#define STP_TYPE_TCN 0x80			// indicating STP TCN packet

// STP header
struct stp_header {
	u16 proto_id;	// STP_PROTOCOL_ID
	u8 version;		// STP_PROTOCOL_VERSION
	u8 msg_type;	// STP_TYPE_CONFIG in this lab
}__attribute__((packed));

// STP Config flags, useless in this lab
#define STP_CONFIG_TOPO_CHANGE 0x01
#define STP_CONFIG_TOPO_CHANGE_ACK 0x80

// STP Config packet
struct stp_config {
	struct stp_header header;
	u8 flags;			// set to 0 in this lab
	u64 root_id;		// root switch (it believes)
	u32 root_path_cost;	// cost of path to root
	u64 switch_id;		// switch sending this packet
	u16 port_id;		// port sending this packet
	u16 msg_age;		// age of STP packet at tx time
	u16 max_age;		// timeout for received data: STP_MAX_AGE
	u16 hello_time;		// time between STP packet generation: STP_HELLO_TIME
	u16 fwd_delay;		// delay between states: STP_FWD_DELAY, useless in this lab
}__attribute__((packed));

// STP Topo Change Notification packet, useless in this lab
struct stp_tcn {
	struct stp_header header;
}__attribute__((packed));

#endif
