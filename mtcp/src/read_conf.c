#include "read_conf.h"
/* all ip in network order */

int count_dict_entries(FILE *file) 
{
    int count = 0;
    char line[100];
    long start_pos = ftell(file);

    while (fgets(line, sizeof(line), file) != NULL && strstr(line, "}") == NULL) {
        if (strstr(line, "=") != NULL) {
            count++;
        }
    }

    fseek(file, start_pos, SEEK_SET); // Move back to the start position
    return count;
}

/* read ip in network byteorder*/
void read_ip_dict(FILE *file, IpEntry *ipEntries, int size) 
{
    char line[100];
    uint8_t ip[4];
    int fscanf_ret;
    for (int i = 0; i < size; i++) {
        fscanf_ret = fscanf(file, " %[^ =] = %hhu.%hhu.%hhu.%hhu", ipEntries[i].key, &ip[0], &ip[1], &ip[2], &ip[3]);
        if (fscanf_ret != 5 )
        {
            TRACE_CONFIG("Error in read IP: %d\n", fscanf_ret);
        }
        ipEntries[i].value = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
        if(fgets(line, sizeof(line), file) == NULL) // Read rest of the line
        {
            TRACE_CONFIG("Error in read next line\n");
        } 
    }
}

void read_mac_dict(FILE *file, MacEntry *macEntries, int size) 
{
    char line[100];
    char mac_str[20];
    int fscanf_ret;
    for (int i = 0; i < size; i++) {
        fscanf_ret = fscanf(file, " %[^ =] = %s", macEntries[i].key, mac_str);
        if (fscanf_ret != 2)
        {
            TRACE_CONFIG("Error in read MAC: %d\n", fscanf_ret);
        }
        if (fgets(line, sizeof(line), file) == NULL) // Read rest of the line
        {
           TRACE_CONFIG("Error in read next line\n"); 
        } 
        sscanf(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &macEntries[i].mac[0], &macEntries[i].mac[1],
               &macEntries[i].mac[2], &macEntries[i].mac[3],
               &macEntries[i].mac[4], &macEntries[i].mac[5]);
    }
}

void read_url_dict(FILE *file, UrlEntry *urlEntries, int size)
{
    char line[100];
    int fscanf_ret;
    for (int i = 0; i < size; i++) {
        fscanf_ret = fscanf(file, " %[^ =] = %[^/]/%s", urlEntries[i].key, urlEntries[i].host, urlEntries[i].path);
        if (fscanf_ret != 3) TRACE_CONFIG("Error in read URL: %d\n", fscanf_ret);
        if (fgets(line, sizeof(line), file) == NULL) // Read rest of the line
        {
            TRACE_CONFIG("Error in read next line\n");
        }
    }
}

void read_vxlan_dict(FILE *file, VxlanEntry *vxlanEntries) {
    char line[100];
    while(fgets(line, sizeof(line), file) != NULL && strstr(line, "}") == NULL)
    {
        char key[10];
        char value[20];
        uint8_t ip[4];
        if (sscanf(line, " %[^ =] = %s", key, value) == 2) {
            if (strcmp(key, "PORT") == 0) {
                vxlanEntries->PORT = (uint16_t)atoi(value);
            } else if (strcmp(key, "FLAG") == 0) {
                vxlanEntries->FLAG = (uint8_t)strtol(value, NULL, 16); // 16进制
            } else if (strcmp(key, "VNI") == 0) {
                vxlanEntries->VNI = (uint16_t)atoi(value);
            } else if (strcmp(key, "IP_PC1") == 0) {
                sscanf(value, "%hhu.%hhu.%hhu.%hhu", &ip[0], &ip[1], &ip[2], &ip[3]);
                vxlanEntries->IP_PC1 = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
            } else if (strcmp(key, "IP_PC3") == 0) {
                sscanf(value, "%hhu.%hhu.%hhu.%hhu", &ip[0], &ip[1], &ip[2], &ip[3]);
                vxlanEntries->IP_PC3 = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
            } else if (strcmp(key, "MAC_PC1") == 0) {
                sscanf(value, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &vxlanEntries->MAC_PC1[0], &vxlanEntries->MAC_PC1[1], &vxlanEntries->MAC_PC1[2], &vxlanEntries->MAC_PC1[3], &vxlanEntries->MAC_PC1[4], &vxlanEntries->MAC_PC1[5]);
            } else if (strcmp(key, "MAC_PC3") == 0) {
                sscanf(value, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &vxlanEntries->MAC_PC3[0], &vxlanEntries->MAC_PC3[1], &vxlanEntries->MAC_PC3[2], &vxlanEntries->MAC_PC3[3], &vxlanEntries->MAC_PC3[4], &vxlanEntries->MAC_PC3[5]);
            }
        }
    }   
}

void read_func_dict(FILE *file, FuncEntry *funcEntries) {
    char line[100];
    while(fgets(line, sizeof(line), file) != NULL && strstr(line, "}") == NULL)
    {
        char key[50];
        char value[20];
        uint8_t ip[4];
        if (sscanf(line, " %[^ =] = %s", key, value) == 2) {
            if (strcmp(key, "FAULT_INJECTION") == 0) {
                funcEntries->fault_injection = (int)atoi(value);
            } else if (strcmp(key, "ROUTING") == 0) {
                funcEntries->routing = (int)atoi(value);
            } else if (strcmp(key, "SERVER_IP") == 0 ){
                sscanf(value, "%hhu.%hhu.%hhu.%hhu", &ip[0], &ip[1], &ip[2], &ip[3]);
                funcEntries->server_ip = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
            } else if (strcmp(key, "SERVER_MAC") == 0){
                sscanf(value, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",&funcEntries->server_mac[0], &funcEntries->server_mac[1], &funcEntries->server_mac[2], &funcEntries->server_mac[3], &funcEntries->server_mac[4], &funcEntries->server_mac[5]);
            }
        }
    }   
}

void process_ip_dict(FILE *file) 
{
    int ip_size = count_dict_entries(file);
    IpEntry *ipEntries = (IpEntry *)malloc(ip_size * sizeof(IpEntry));
    read_ip_dict(file, ipEntries, ip_size);
    config_dict->len = ip_size;
    config_dict->ip_dict = ipEntries;
}

void process_mac_dict(FILE *file) 
{
    int mac_size = count_dict_entries(file);
    MacEntry *macEntries = (MacEntry *)malloc(mac_size * sizeof(MacEntry));
    read_mac_dict(file, macEntries, mac_size);
    config_dict->mac_dict = macEntries;
}

void process_url_dict(FILE *file)
{
    int url_size = count_dict_entries(file);
    UrlEntry* urlEntries = (UrlEntry*)malloc(url_size * sizeof(UrlEntry));
    read_url_dict(file, urlEntries, url_size);
    config_dict->url_len = url_size;
    config_dict->url_dict = urlEntries;
}

void process_vxlan_dict(FILE *file)
{
    VxlanEntry* vxlanEntries = (VxlanEntry*)malloc(sizeof(VxlanEntry));
    read_vxlan_dict(file, vxlanEntries);
    config_dict->vxlan_dict = vxlanEntries;
}

void process_func_dict(FILE *file)
{
    FuncEntry* funcEntries = (FuncEntry*)malloc(sizeof(FuncEntry));
    read_func_dict(file, funcEntries);
    config_dict->func_dict = funcEntries;
}

void print_config_dict(const Config_dict *dict) 
{
    printf("IP Dictionary (length: %d):\n", dict->len);
    for (int i = 0; i < dict->len; i++) {
        printf("\tKey: %s, Value: %u.%u.%u.%u\n", 
               dict->ip_dict[i].key, 
               (dict->ip_dict[i].value & 0xFF000000) >> 24,
               (dict->ip_dict[i].value & 0x00FF0000) >> 16,
               (dict->ip_dict[i].value & 0x0000FF00) >> 8,
               (dict->ip_dict[i].value & 0x000000FF));
    }
    printf("\nMAC Dictionary (length: %d):\n", dict->len);
    for (int i = 0; i < dict->len; i++) {
        printf("\tKey: %s, Value: %02x:%02x:%02x:%02x:%02x:%02x\n", 
               dict->mac_dict[i].key,
               dict->mac_dict[i].mac[0],
               dict->mac_dict[i].mac[1],
               dict->mac_dict[i].mac[2],
               dict->mac_dict[i].mac[3],
               dict->mac_dict[i].mac[4],
               dict->mac_dict[i].mac[5]);
    }
    printf("\nURL Dictionary (length: %d):\n", dict->url_len);
    for (int i = 0; i < dict->url_len; i++) {
        printf("\tKey: %s, Host: %s, Path: %s\n",
                dict->url_dict[i].key,
                dict->url_dict[i].host,
                dict->url_dict[i].path);
    }
    
    printf("\nVxlan Dictionary:\n");
    printf("\tPORT: %u\n", dict->vxlan_dict->PORT);
    printf("\tFLAG: 0x%x\n", dict->vxlan_dict->FLAG);
    printf("\tVNI: %u\n", dict->vxlan_dict->VNI);
    printf("\tIP_PC1: %u.%u.%u.%u\n",
           (dict->vxlan_dict->IP_PC1 >> 24) & 0xFF,
           (dict->vxlan_dict->IP_PC1 >> 16) & 0xFF,
           (dict->vxlan_dict->IP_PC1 >> 8) & 0xFF,
           dict->vxlan_dict->IP_PC1 & 0xFF);
    printf("\tIP_PC3: %u.%u.%u.%u\n",
           (dict->vxlan_dict->IP_PC3 >> 24) & 0xFF,
           (dict->vxlan_dict->IP_PC3 >> 16) & 0xFF,
           (dict->vxlan_dict->IP_PC3 >> 8) & 0xFF,
           dict->vxlan_dict->IP_PC3 & 0xFF);
    printf("\tMAC_PC1: %02x:%02x:%02x:%02x:%02x:%02x\n",
           dict->vxlan_dict->MAC_PC1[0], dict->vxlan_dict->MAC_PC1[1], dict->vxlan_dict->MAC_PC1[2],
           dict->vxlan_dict->MAC_PC1[3], dict->vxlan_dict->MAC_PC1[4], dict->vxlan_dict->MAC_PC1[5]);
    printf("\tMAC_PC3: %02x:%02x:%02x:%02x:%02x:%02x\n",
           dict->vxlan_dict->MAC_PC3[0], dict->vxlan_dict->MAC_PC3[1], dict->vxlan_dict->MAC_PC3[2],
           dict->vxlan_dict->MAC_PC3[3], dict->vxlan_dict->MAC_PC3[4], dict->vxlan_dict->MAC_PC3[5]);
    
    printf("\nFunc Dictionary:\n");
    printf("\tFAULT_INJECTION: %d\n", dict->func_dict->fault_injection);
    printf("\tROUTING: %d\n", dict->func_dict->routing);
    printf("\tSERVER_IP: %u.%u.%u.%u\n", 
        (dict->func_dict->server_ip >> 24) & 0xFF,
        (dict->func_dict->server_ip >> 16) & 0xFF,
        (dict->func_dict->server_ip >> 8) & 0xFF,
        dict->func_dict->server_ip & 0xFF);
    printf("\tSERVER_MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
        dict->func_dict->server_mac[0],dict->func_dict->server_mac[1],dict->func_dict->server_mac[2],
        dict->func_dict->server_mac[3],dict->func_dict->server_mac[4],dict->func_dict->server_mac[5]);
}

int read_config()
{
    FILE *file = fopen("/home/test/projects/mtcp/mtcp/apps/proxy/config/p4mesh.conf", "r");
    if (file == NULL) {
        perror("Failed to open file");
        return 1;
    }

    config_dict = (Config_dict*)malloc(sizeof(Config_dict));
    char line[100];
    while (fgets(line, sizeof(line), file) != NULL) {
        if (strstr(line, "ip_dict = {")) {
            // fgets(line, sizeof(line), file); // Read next line to skip the "{" line
            process_ip_dict(file);
        } else if (strstr(line, "mac_dict = {")) {
            // fgets(line, sizeof(line), file); // Read next line to skip the "{" line
            process_mac_dict(file);
        } else if (strstr(line, "url_dict = {")) {
            process_url_dict(file);
        } else if (strstr(line, "vxlan_dict = {")) {
            process_vxlan_dict(file);
        } else if (strstr(line, "func_dict = {")) {
            process_func_dict(file);
        }
    }
    print_config_dict(config_dict);
    fclose(file);

    return 0;
}