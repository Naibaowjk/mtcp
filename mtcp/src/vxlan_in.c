#include "vxlan_in.h"


int
ProcessVxlanPacket(struct mtcp_manager *mtcp, uint32_t cur_ts, 
				  const int ifidx, const struct udphdr* udph, int len)
{
    MEASURE_START();
    struct vxlanhdr* vxlanh = (struct vxlanhdr*)(udph + 1);
    unsigned char * pktdata = (unsigned char *)(vxlanh + 1);
    uint32_t vni_n = ntohl(vxlanh->vni << 8);
    /* check if the vni is 1131 */
    if (vni_n == config_dict->vxlan_dict->VNI)
    {
        TRACE_CONFIG("successfully check vxlan VNI:%d\n", config_dict->vxlan_dict->VNI);
        return ProcessPacket(mtcp, ifidx, cur_ts, pktdata, len - 8);
    }

    /* error! should not be there */
    MEASURE_END("ProcessVxlanPacket");
    return FALSE;

}