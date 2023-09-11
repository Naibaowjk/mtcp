#include "vxlan_out.h"


uint8_t *
VxlanOutput(struct mtcp_manager *mtcp, uint16_t inner_len, int ifidx)
{
    struct vxlanhdr* vxlan_hdr;
	uint16_t vxlan_len;
	VxlanEntry* vxlanEntries = config_dict->vxlan_dict;

	vxlan_len = inner_len + 8;
	vxlan_hdr = (struct vxlanhdr*)UDPOutput(mtcp, vxlan_len, ifidx);
	vxlan_hdr->flags = (uint8_t)(vxlanEntries->FLAG & 0xFF);
	vxlan_hdr->vni = htons((uint16_t)vxlanEntries->VNI) << 8;
	vxlan_hdr->reserved1 = 0; 
	vxlan_hdr->reserved2 = 0; 

	return (uint8_t *)(vxlan_hdr + 1);
}