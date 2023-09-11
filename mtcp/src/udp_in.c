#include "udp_in.h"


int
ProcessUDPPacket(struct mtcp_manager *mtcp, uint32_t cur_ts, const int ifidx, const struct iphdr* iph, int ip_len)
{
   struct udphdr* udph = (struct udphdr*) IP_NEXT_PTR(iph);
   int payload_len = ntohs(udph->len) - 8;

   /* check udp port is from vxlan */
   if (ntohs(udph->dest) == config_dict->vxlan_dict->PORT)
   {
      /* it's is vxlan header */
      TRACE_CONFIG("successfully check vxlan dst port: %d\n", config_dict->vxlan_dict->PORT);
      return ProcessVxlanPacket(mtcp, cur_ts, ifidx, udph, payload_len);
   }
   else // drop other UDP pakcet
   {
      return FALSE;
   }
   /* should not go there */ 
   return FALSE;
}