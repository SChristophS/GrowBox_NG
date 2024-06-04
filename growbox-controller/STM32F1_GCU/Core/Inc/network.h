#ifndef NETWORK_H
#define NETWORK_H

#include "wizchip_conf.h"

// Network configuration and initialization function
void initialize_network(void);
void print_network_information(void);
int8_t process_dhcp(void);
int8_t process_dns(void);

int32_t loopback_tcpc(uint8_t sn, uint8_t* buf, uint8_t* destip, uint16_t destport);

#define OFF 0
#define ON 1

#define DATA_BUF_SIZE 2048

#define SOCK_DHCP 3
#define SOCK_DNS 4


#endif // NETWORK_H
