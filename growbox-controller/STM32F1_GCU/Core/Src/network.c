#include "network.h"
#include <stdio.h>
#include "dhcp/dhcp.h"
#include "dns/dns.h"
#include "wizchip_init.h"
#include "uart_redirect.h"

// Network configuration
wiz_NetInfo defaultNetInfo = {
    .mac = {0x00,0x08,0xdc,0xff,0xee,0xdd},
    .ip = {192,168,178,100},
    .sn = {255,255,255,0},
    .gw = {192,168,178,1},
    .dns = {8, 8, 8, 8},
    .dhcp = NETINFO_STATIC
};

#define DATA_BUF_SIZE 2048
#define SOCK_DHCP 3
#define SOCK_DNS 4

uint8_t g_send_buf[DATA_BUF_SIZE];
uint8_t g_recv_buf[DATA_BUF_SIZE];
uint8_t data_buf[DATA_BUF_SIZE];

uint8_t dns_server[4] = {168, 126, 63, 1}; // Secondary DNS server IP
uint8_t Domain_IP[4] = {0}; // Translated IP address by DNS Server
uint8_t Domain_name[] = "www.google.com";
uint8_t flag_process_dhcp_success = OFF;
uint8_t flag_process_dns_success = OFF;

void print_network_information(void) {
    wizchip_getnetinfo(&defaultNetInfo);
    printf("Mac address: %02x:%02x:%02x:%02x:%02x:%02x\n\r", defaultNetInfo.mac[0], defaultNetInfo.mac[1], defaultNetInfo.mac[2], defaultNetInfo.mac[3], defaultNetInfo.mac[4], defaultNetInfo.mac[5]);
    printf("IP address : %d.%d.%d.%d\n\r", defaultNetInfo.ip[0], defaultNetInfo.ip[1], defaultNetInfo.ip[2], defaultNetInfo.ip[3]);
    printf("SM Mask    : %d.%d.%d.%d\n\r", defaultNetInfo.sn[0], defaultNetInfo.sn[1], defaultNetInfo.sn[2], defaultNetInfo.sn[3]);
    printf("Gate way   : %d.%d.%d.%d\n\r", defaultNetInfo.gw[0], defaultNetInfo.gw[1], defaultNetInfo.gw[2], defaultNetInfo.gw[3]);
    printf("DNS Server : %d.%d.%d.%d\n\r", defaultNetInfo.dns[0], defaultNetInfo.dns[1], defaultNetInfo.dns[2], defaultNetInfo.dns[3]);
}

int8_t process_dhcp(void) {
    uint8_t ret = 0;
    uint8_t dhcp_retry = 0;

    printf(" - DHCP Client running\r\n");
    DHCP_init(SOCK_DHCP, data_buf);

    printf(" - DHCP Init done\r\n");
    while (1) {
        ret = DHCP_run();
        if (ret == DHCP_IP_LEASED) {
            printf(" - DHCP Success\r\n");
            break;
        } else if (ret == DHCP_FAILED) {
            dhcp_retry++;
            if (dhcp_retry <= 3) printf(" - DHCP Timeout occurred and retry [%d]\r\n", dhcp_retry);
        }
        if (dhcp_retry > 3) {
            printf(" - DHCP Failed\r\n\r\n");
            DHCP_stop();
            break;
        }
    }
    return ret;
}

int8_t process_dns(void) {
    int8_t ret = 0;
    uint8_t dns_retry = 0;

    printf(" - DNS Client running\r\n");
    DNS_init(SOCK_DNS, data_buf);
    while (1) {
        if ((ret = DNS_run(dns_server, (uint8_t *)Domain_name, Domain_IP)) == 1) {
            printf(" - DNS Success\r\n");
            break;
        } else {
            dns_retry++;
            if (dns_retry <= 2) printf(" - DNS Timeout occurred and retry [%d]\r\n", dns_retry);
        }
        if (dns_retry > 2) {
            printf(" - DNS Failed\r\n\r\n");
            break;
        }
    }
    return ret;
}

void initialize_network(void) {
    printf("\t - WizChip Init - \r\n");
    WIZCHIPInitialize();
    printf("version:%.2x\r\n", getVERSIONR());

    wizchip_setnetinfo(&defaultNetInfo);
    print_network_information();

    if (process_dhcp() == DHCP_IP_LEASED) {
        flag_process_dhcp_success = ON;
    } else {
        ctlnetwork(CN_SET_NETINFO, &defaultNetInfo); // Set default static IP settings
    }


    printf("Register value after W5x00 initialize!\r\n");
    print_network_information();

    if (process_dns()) {
        flag_process_dns_success = ON;
    }

    if (flag_process_dhcp_success == ENABLE) {
        printf(" # DHCP IP Leased time : %lu seconds\r\n", getDHCPLeasetime());
    } else {
        printf(" # DHCP Failed\r\n");
    }

    if (flag_process_dns_success == ENABLE) {
        printf(" # DNS: %s => %d.%d.%d.%d\r\n", Domain_name, Domain_IP[0], Domain_IP[1], Domain_IP[2], Domain_IP[3]);
    } else {
        printf(" # DNS Failed\r\n");
    }
}