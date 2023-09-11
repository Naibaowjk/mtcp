#ifndef UDP_IN_H
#define UDP_IN_H

#include <linux/udp.h>
#include <netinet/ip.h>
#include "read_conf.h"
#include "mtcp_api.h"
#include "mtcp.h"
#include "vxlan_in.h"

#ifndef UDP_LEN
#define UDP_LEN 64
#endif //UDP_LEN

#ifndef IP_NEXT_PTR
#define IP_NEXT_PTR(iph) ((uint8_t *)iph + (iph->ihl << 2))
#endif //IP_NEXT_PTR

int
ProcessUDPPacket(struct mtcp_manager *mtcp, uint32_t cur_ts, 
				  const int ifidx, const struct iphdr* iph, int ip_len);
#endif //UDP_IN_H