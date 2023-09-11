#include <stdio.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_common.h>

int main(int argc, char *argv[]) {
    int ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        fprintf(stderr, "Error with EAL initialization\n");
        return -1;
    }

    uint16_t port_id = 0; // Assume the port you're interested in is 0; adjust as needed
    
    // 配置结构体
    struct rte_eth_conf port_conf = {
        .rxmode = { .mq_mode = ETH_MQ_RX_RSS },
        .txmode = { .mq_mode = ETH_MQ_TX_NONE },
        .fc = {
            .autoneg = 0,
            .mode = RTE_FC_FULL,
        },
    };

    // 端口配置
    ret = rte_eth_dev_configure(port_id, 1, 1, &port_conf);
    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%u\n", ret, port_id);
    }
    struct rte_eth_fc_conf fc_conf;

    int result = rte_eth_dev_flow_ctrl_get(port_id, &fc_conf);
    if (result == 0) {
        printf("Successfully retrieved flow control settings\n");
    } else if (result == -ENOTSUP) {
        printf("Hardware doesn't support flow control: %d\n", result);
    } else if (result == -ENODEV) {
        printf("Invalid port ID: %d\n", result);
    } else {
        printf("An unknown error occurred: %d\n", result);
    }

    return 0;
}
