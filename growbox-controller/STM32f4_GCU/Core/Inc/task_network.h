#ifndef NETWORK_TASK_H
#define NETWORK_TASK_H

#include "wizchip_conf.h"
#include "cmsis_os2.h"

extern osMessageQueueId_t xMessageQueueHandle;


#define DATA_BUF_SIZE 2048
#define SOCK_DHCP 3


int32_t loopback_tcpc(uint8_t sn, uint8_t* buf, uint8_t* destip, uint16_t destport);

#define OFF 0
#define ON 1

//#define DATA_BUF_SIZE 2048

#define JSON_TOKENS 128

//#define SOCK_DHCP 3
#define SOCK_DNS 4

#define PING_INTERVAL 2000 // Interval in milliseconds to send ping
#define WEBSOCKET_PING_OPCODE 0x9 // WebSocket Ping Opcode
#define WEBSOCKET_MASK_KEY_SIZE 4


// Network configuration and initialization function
void initialize_network(void);
void print_network_information(void);
int8_t process_dhcp(void);
int8_t process_dns(void);
int upgrade_to_websocket(uint8_t sn);

void StartNetworkTask(void *argument);

#endif // NETWORK_TASK_H
