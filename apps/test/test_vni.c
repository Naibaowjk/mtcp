#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>


struct vxlanhdr {
    uint8_t flags;
    uint32_t reserved1:24;
    uint32_t vni:24;
    uint8_t reserved2;
};

int main() {
    struct vxlanhdr vxlan_header;
    vxlan_header.vni = vxlan_header.vni = 0x6B0400; // 假设这是一个从网络上接收到的 vxlan header，vni 是网络字节序

    uint32_t my_value = 0x0000046B; // 这是一个主机字节序的值

    uint32_t v1 = vxlan_header.vni << 8;
    uint32_t v2 = ntohl(v1);
    printf("v1 = %u\nv2 = %u\n", v1, v2);
    printf("myvalue = %u\n", my_value);


    // 现在，两个值都是主机字节序，我们可以直接比较它们
    if (v2 == my_value) {
        printf("The two values are equal!\n");
    } else {
        printf("The two values are not equal.\n");
    }

    return 0;
}
