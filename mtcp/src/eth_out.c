#include <stdio.h>

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include <linux/if_ether.h>
#include <linux/tcp.h>
#include <netinet/ip.h>

#include "mtcp.h"
#include "arp.h"
#include "eth_out.h"
#include "debug.h"
#include "vxlan_out.h"

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef ERROR
#define ERROR (-1)
#endif

#define MAX(a, b) ((a)>(b)?(a):(b))
#define MIN(a, b) ((a)<(b)?(a):(b))

#define MAX_WINDOW_SIZE 65535

/*----------------------------------------------------------------------------*/
uint8_t *
EthernetOutput(struct mtcp_manager *mtcp, uint16_t h_proto, 
		int nif, unsigned char* src_haddr, unsigned char* dst_haddr, uint16_t iplen, int is_inner)
{
	uint8_t *buf;
	struct ethhdr *ethh;
	int i, eidx;

	/* 
 	 * -sanity check- 
	 * return early if no interface is set (if routing entry does not exist)
	 */
	if (nif < 0) {
		TRACE_INFO("No interface set!\n");
		return NULL;
	}

	eidx = CONFIG.nif_to_eidx[nif];
	if (eidx < 0) {
		TRACE_INFO("No interface selected!\n");
		return NULL;
	}
	
	if (is_inner == 0)
	{
		buf = mtcp->iom->get_wptr(mtcp->ctx, eidx, iplen + ETHERNET_HEADER_LEN);
		if (!buf) {
			TRACE_CONFIG("Failed to get available write buffer\n");
			return NULL;
		}
		ethh = (struct ethhdr *)buf;
	}
	else  // should use vxlan
	{
		ethh = (struct ethhdr*)VxlanOutput(mtcp, iplen + ETHERNET_HEADER_LEN, eidx);
	}
	//memset(buf, 0, ETHERNET_HEADER_LEN + iplen);

#if 0
	TRACE_DBG("dst_hwaddr: %02X:%02X:%02X:%02X:%02X:%02X\n",
				dst_haddr[0], dst_haddr[1], 
				dst_haddr[2], dst_haddr[3], 
				dst_haddr[4], dst_haddr[5]);
#endif
	
	/* for ARP situation, we should check src_haddr same as old */
	
	for (i = 0; i < ETH_ALEN; i++) {
		ethh->h_source[i] = src_haddr == NULL? CONFIG.eths[eidx].haddr[i] : src_haddr[i];
		ethh->h_dest[i] = dst_haddr[i];
	}

	TRACE_CONFIG("src_hwaddr: %02X:%02X:%02X:%02X:%02X:%02X\n",
		CONFIG.eths[eidx].haddr[0], CONFIG.eths[eidx].haddr[1], 
		CONFIG.eths[eidx].haddr[2], CONFIG.eths[eidx].haddr[3], 
		CONFIG.eths[eidx].haddr[4], CONFIG.eths[eidx].haddr[5]);

	TRACE_CONFIG("dst_hwaddr: %02X:%02X:%02X:%02X:%02X:%02X\n",
		dst_haddr[0], dst_haddr[1], 
		dst_haddr[2], dst_haddr[3], 
		dst_haddr[4], dst_haddr[5]);


	ethh->h_proto = htons(h_proto);
	return (uint8_t *)(ethh + 1);
}
/*----------------------------------------------------------------------------*/
