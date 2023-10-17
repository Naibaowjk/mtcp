#ifndef READ_CONF_H
#define READ_CONF_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "debug.h"

#define MAX_KEY_LENGTH 50

typedef struct {
    char key[MAX_KEY_LENGTH];
    uint32_t value;
} IpEntry;

typedef struct {
    char key[MAX_KEY_LENGTH];
    uint8_t mac[6];
} MacEntry;

typedef struct {
    char key[50];
    char host[50];
    char path[10];
} UrlEntry;

typedef struct {
    uint16_t PORT;
    uint8_t FLAG;
    uint16_t VNI;
    uint32_t IP_PC1;
    uint32_t IP_PC3;
    uint8_t MAC_PC1[6];
    uint8_t MAC_PC3[6];
} VxlanEntry;

typedef struct {
    int fault_injection;
    int routing;
    int load_balance;
    uint32_t server_ip;
    uint8_t server_mac[6];
} FuncEntry;

typedef struct {
    int len;
    int url_len; // url is only in server.
    IpEntry* ip_dict;
    MacEntry* mac_dict;
    UrlEntry* url_dict; 
    VxlanEntry* vxlan_dict;
    FuncEntry* func_dict;
} Config_dict;


Config_dict* config_dict;

int count_dict_entries(FILE *file);

void read_ip_dict(FILE *file, IpEntry *ipEntries, int size);

void read_mac_dict(FILE *file, MacEntry *macEntries, int size);

void read_url_dict(FILE *file, UrlEntry *urlEntries, int size);

void read_vxlan_dict(FILE *file, VxlanEntry *vxlanEntries);

void read_func_dict(FILE *file, FuncEntry *funcEntries);

void process_ip_dict(FILE *file);

void process_mac_dict(FILE *file);

void process_url_dict(FILE *file);

void process_vxlan_dict(FILE *file);

void process_func_dict(FILE *file);

void print_config_dict(const Config_dict *dict);

int read_config();

#endif