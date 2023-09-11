#ifndef ETH_OUT_H
#define ETH_OUT_H

#include <stdint.h>

#include "mtcp.h"
#include "tcp_stream.h"
#include "ps.h"

#define MAX_SEND_PCK_CHUNK 64

uint8_t *
EthernetOutput(struct mtcp_manager *mtcp, uint16_t h_proto, 
		int nif, unsigned char* src_haddr, unsigned char* dst_haddr, uint16_t iplen, int is_inner);

#endif /* ETH_OUT_H */
