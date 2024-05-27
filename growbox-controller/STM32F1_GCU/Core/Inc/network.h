#ifndef NETWORK_H
#define NETWORK_H

#include "wizchip_conf.h"

// Network configuration and initialization function
void initialize_network(void);
void print_network_information(void);
int8_t process_dhcp(void);
int8_t process_dns(void);

#define OFF 0
#define ON 1

#endif // NETWORK_H
