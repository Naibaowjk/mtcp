#include "read_conf.h"
#ifndef VXLAN_OUT
#define VXLAN_OUT
#include "vxlan_in.h"
#include "mtcp.h"
#include "debug.h"
#include "udp_out.h"



uint8_t *
VxlanOutput(struct mtcp_manager *mtcp, uint16_t inner_len, int ifidx);
#endif //VXLAN_OUT