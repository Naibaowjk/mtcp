#include <stdio.h>
#include <stdint.h>
#include <rte_eal.h>
#include <rte_ethdev.h>

#define RSS_KEY_LEN 40  // 这是一个常见的RSS key长度。根据你的NIC，可能需要调整。

int main(int argc, char *argv[]) {
    int ret;
    uint8_t rss_key[RSS_KEY_LEN];
    struct rte_eth_rss_conf rss_conf;
    
    // 初始化DPDK环境
    ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
    }
    
    // 确保至少有一个设备可用
    if (rte_eth_dev_count_avail() == 0) {
        rte_exit(EXIT_FAILURE, "No available eth devices\n");
    }

    // 使用第一个设备作为示例
    uint16_t port_id = 0;

    rss_conf.rss_key = rss_key;
    rss_conf.rss_key_len = RSS_KEY_LEN;

    // 获取RSS配置
    ret = rte_eth_dev_rss_hash_conf_get(port_id, &rss_conf);
    if (ret != 0) {
        rte_exit(EXIT_FAILURE, "Failed to get RSS hash configuration\n");
    }

    // 打印RSS key
    printf("RSS Key for port %u:\n", port_id);
    for (int i = 0; i < rss_conf.rss_key_len; i++) {
        printf("%02X ", rss_key[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");

    return 0;
}