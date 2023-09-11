struct rte_eth_conf port_conf = {
#ifndef T4P4S_VETH_MODE
    .rxmode = {
#if RTE_VERSION >= RTE_VERSION_NUM(22,03,0,0)
        .mq_mode = RTE_ETH_MQ_RX_RSS,
#else
        .mq_mode = ETH_MQ_RX_RSS,
#endif
#if RTE_VERSION >= RTE_VERSION_NUM(21,11,0,0)
        .mtu = RTE_ETH_MAX_LEN,
#else
        .max_rx_pkt_len = RTE_ETH_MAX_LEN,
#endif
        .split_hdr_size = 0,
#if RTE_VERSION >= RTE_VERSION_NUM(22,03,0,0)
    #ifndef T4P4S_RTE_OFFLOADS
        #define T4P4S_RTE_OFFLOADS RTE_ETH_RX_OFFLOAD_CHECKSUM
    #endif
        .offloads = T4P4S_RTE_OFFLOADS,
#elif RTE_VERSION >= RTE_VERSION_NUM(18,11,0,0)
    #ifndef T4P4S_RTE_OFFLOADS
        #define T4P4S_RTE_OFFLOADS DEV_RX_OFFLOAD_CHECKSUM
    #endif
        .offloads = T4P4S_RTE_OFFLOADS,
#elif RTE_VERSION >= RTE_VERSION_NUM(18,05,0,0)
        .offloads = DEV_RX_OFFLOAD_CRC_STRIP | DEV_RX_OFFLOAD_CHECKSUM,
#else
        .header_split   = 0, // Header Split disabled
        .hw_ip_checksum = 1, // IP checksum offload enabled
        .hw_vlan_filter = 0, // VLAN filtering disabled
        .jumbo_frame    = 0, // Jumbo Frame Support disabled
        .hw_strip_crc   = 1, // CRC stripped by hardware
#endif
    },
    .rx_adv_conf = {
        .rss_conf = {
            .rss_key = NULL,
            .rss_hf = T4P4S_RTE_RSS_HF,
        },
    },
    .txmode = {
#if RTE_VERSION >= RTE_VERSION_NUM(22,03,0,0)
        .mq_mode = RTE_ETH_MQ_TX_NONE,
#else
        .mq_mode = ETH_MQ_TX_NONE,
#endif
    },
#endif
};