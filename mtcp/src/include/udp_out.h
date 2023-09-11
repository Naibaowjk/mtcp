#ifndef UDP_OUT_H
#define UDP_OUT_H
#include "read_conf.h"
#include "debug.h"
#include "ip_out.h"

uint8_t *
UDPOutput(struct mtcp_manager *mtcp, uint16_t vxlan_len, int ifidx);


#endif // UDP_OUT_H