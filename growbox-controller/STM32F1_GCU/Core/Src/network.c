#include "network.h"
#include <stdio.h>
#include "dhcp/dhcp.h"
#include "dns/dns.h"
#include "wizchip_init.h"
#include "uart_redirect.h"
#include "socket.h" // Falls notwendig, anpassen je nach Bibliothek
#include <string.h>

// Network configuration
wiz_NetInfo defaultNetInfo = {
    .mac = {0x00,0x08,0xdc,0xff,0xee,0xdd},
    .ip = {192,168,178,100},
    .sn = {255,255,255,0},
    .gw = {192,168,178,1},
    .dns = {8, 8, 8, 8},
    .dhcp = NETINFO_STATIC
};

uint8_t g_send_buf[DATA_BUF_SIZE];
uint8_t g_recv_buf[DATA_BUF_SIZE];
uint8_t data_buf[DATA_BUF_SIZE];

uint8_t dns_server[4] = {168, 126, 63, 1}; // Secondary DNS server IP
uint8_t Domain_IP[4] = {0}; // Translated IP address by DNS Server
uint8_t Domain_name[] = "www.google.com";

uint8_t flag_process_dhcp_success = OFF;
uint8_t flag_process_dns_success = OFF;

void GetSTM32UID(char *uidStr) {
    uint32_t uid[3];
    uid[0] = *(uint32_t *)0x1FFFF7E8;
    uid[1] = *(uint32_t *)0x1FFFF7EC;
    uid[2] = *(uint32_t *)0x1FFFF7F0;

    sprintf(uidStr, "%08lX%08lX%08lX", uid[0], uid[1], uid[2]);
}

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

    // UID auslesen und anzeigen
    char uidStr[25];
    GetSTM32UID(uidStr);
    printf("STM32 UID: %s\n", uidStr);
}

void upgrade_to_websocket(uint8_t sn) {
    char request[] = "GET /chat HTTP/1.1\r\n"
                     "Host: example.com\r\n"
                     "Upgrade: websocket\r\n"
                     "Connection: Upgrade\r\n"
                     "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n"
                     "Sec-WebSocket-Version: 13\r\n\r\n";

    send(sn, (uint8_t*)request, strlen(request));

    uint8_t response[1024];
    int32_t len = recv(sn, response, sizeof(response));
    response[len] = '\0';
    printf("Server response: %s\n", response);

    // Überprüfe die Antwort auf Erfolg
}

int32_t loopback_tcpc(uint8_t sn, uint8_t* buf, uint8_t* destip, uint16_t destport)
{
   int32_t ret; // return value for SOCK_ERRORs
   uint16_t size = 0, sentsize = 0;
   static uint16_t any_port = 50000;

   switch(getSn_SR(sn))
   {
      case SOCK_ESTABLISHED :
         if(getSn_IR(sn) & Sn_IR_CON)
         {
            printf("%d:Connected to - %d.%d.%d.%d : %d\n", sn, destip[0], destip[1], destip[2], destip[3], destport);
            setSn_IR(sn, Sn_IR_CON);
            // Upgrade auf WebSocket
            upgrade_to_websocket(sn);
         }

         if((size = getSn_RX_RSR(sn)) > 0)
         {
            if(size > DATA_BUF_SIZE) size = DATA_BUF_SIZE;
            ret = recv(sn, buf, size);

            if(ret <= 0) return ret;
            size = (uint16_t) ret;

            // Drucke die empfangene Nachricht
            buf[size] = '\0'; // Stelle sicher, dass der Puffer nullterminiert ist
            printf("Empfangene Nachricht: %s\n", buf);

            sentsize = 0;

            while(size != sentsize)
            {
                ret = send(sn, buf + sentsize, size - sentsize);
                if(ret < 0)
                {
                    close(sn);
                    return ret;
                }
                sentsize += ret;
            }
         }
         break;

      case SOCK_CLOSE_WAIT :
         printf("%d:Socket CloseWait\n", sn);
         if((ret = disconnect(sn)) != SOCK_OK) return ret;
         printf("%d:Socket Closed\n", sn);
         break;

      case SOCK_INIT :
         printf("%d:Try to connect to the %d.%d.%d.%d : %d\n", sn, destip[0], destip[1], destip[2], destip[3], destport);
         if((ret = connect(sn, destip, destport)) != SOCK_OK) return ret;
         break;

      case SOCK_CLOSED:
         printf("%d:Socket closed, reopening...\n", sn);
         if((ret = socket(sn, Sn_MR_TCP, any_port++, 0x00)) != sn)
         {
            if(any_port == 0xffff) any_port = 50000;
            return ret;
         }
         printf("%d:TCP client loopback start\n", sn);
         printf("%d:Socket opened\n", sn);
         break;

      default:
         break;
   }
   return 1;
}
