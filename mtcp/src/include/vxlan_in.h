#ifndef VXLAN_IN
#define VXLAN_IN

#include <linux/udp.h>
#include <sys/time.h>
#include "eth_in.h"
#include "read_conf.h"

/* Vxlan LAYER */
struct vxlanhdr {
    uint8_t flags;
    uint32_t reserved1:24;
    uint32_t vni:24;
    uint8_t reserved2;
};

int
ProcessVxlanPacket(struct mtcp_manager *mtcp, uint32_t cur_ts, 
				  const int ifidx, const struct udphdr* udph, int len);

#endif //VXLAN_IN