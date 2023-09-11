#include "udp_out.h"

uint8_t *
UDPOutput(struct mtcp_manager *mtcp, uint16_t vxlan_len, int ifidx)
{
    int udp_len;
	struct udphdr* udph;
	uint32_t saddr, daddr;
	uint16_t src_port, dst_port;

	VxlanEntry* vxlanEntries;
	vxlanEntries = config_dict->vxlan_dict;

	src_port = 41075; // it should be check from real port, change port to 41075 to check.
	dst_port = config_dict->vxlan_dict->PORT;

    if (ifidx == 0)
    {    
        saddr = vxlanEntries->IP_PC3;
        daddr = vxlanEntries->IP_PC1;
    }
    else
    {
        saddr = vxlanEntries->IP_PC1;
        daddr = vxlanEntries->IP_PC3;
    }

	udp_len = sizeof(struct udphdr) + vxlan_len;

	udph = (struct udphdr*)IPOutputStandalone(mtcp, IPPROTO_UDP, 0, saddr, daddr, udp_len, 0);
	udph->source = htons(src_port);
	udph->dest = htons(dst_port);
	udph->len = htons(udp_len);
	udph->check = htons(0);

	return (uint8_t*)(udph + 1);
}