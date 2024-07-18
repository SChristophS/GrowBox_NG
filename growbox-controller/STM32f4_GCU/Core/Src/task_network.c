#include "task_network.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "uart_redirect.h"
#include "wizchip_init.h"
#include <stdint.h>
#include <stdbool.h>
#include "socket.h"
#include "stdlib.h"

#include "dhcp/dhcp.h"
#include "dns/dns.h"
#include "controller_state.h"

#include "jsmn_utils.h"



// Network configuration - without DHCP
wiz_NetInfo defaultNetInfo = {
    .mac = {0x00,0x08,0xdc,0xff,0xee,0xdd},
    .ip = {192,168,178,100},
    .sn = {255,255,255,0},
    .gw = {192,168,178,1},
    .dns = {8, 8, 8, 8},
    .dhcp = NETINFO_STATIC
};


// variables
uint8_t data_buf[DATA_BUF_SIZE];
uint8_t dns_server[4] = {168, 126, 63, 1}; // Secondary DNS server IP
uint8_t Domain_IP[4] = {0}; // Translated IP address by DNS Server
uint8_t Domain_name[] = "www.google.com";
uint8_t flag_process_dhcp_success = OFF;
uint8_t flag_process_dns_success = OFF;



void print_network_information(void) {
    wizchip_getnetinfo(&defaultNetInfo);
    printf("task_network.c:\t	Mac address: %02x:%02x:%02x:%02x:%02x:%02x\n\r", defaultNetInfo.mac[0], defaultNetInfo.mac[1], defaultNetInfo.mac[2], defaultNetInfo.mac[3], defaultNetInfo.mac[4], defaultNetInfo.mac[5]);
    printf("task_network.c:\t	IP address : %d.%d.%d.%d\n\r", defaultNetInfo.ip[0], defaultNetInfo.ip[1], defaultNetInfo.ip[2], defaultNetInfo.ip[3]);
    printf("task_network.c:\t	SM Mask    : %d.%d.%d.%d\n\r", defaultNetInfo.sn[0], defaultNetInfo.sn[1], defaultNetInfo.sn[2], defaultNetInfo.sn[3]);
    printf("task_network.c:\t	Gate way   : %d.%d.%d.%d\n\r", defaultNetInfo.gw[0], defaultNetInfo.gw[1], defaultNetInfo.gw[2], defaultNetInfo.gw[3]);
    printf("task_network.c:\t	DNS Server : %d.%d.%d.%d\n\r", defaultNetInfo.dns[0], defaultNetInfo.dns[1], defaultNetInfo.dns[2], defaultNetInfo.dns[3]);
}

int8_t process_dhcp(void) {
    uint8_t ret = 0;
    uint8_t dhcp_retry = 0;

    printf("task_network.c:\t DHCP Client running\r\n");

    DHCP_init(SOCK_DHCP, data_buf);

    printf("task_network.c:\t DHCP Init done\r\n");
    while (1) {
        ret = DHCP_run();
        if (ret == DHCP_IP_LEASED) {
        	printf("task_network.c:\t DHCP Success\r\n");
            break;
        } else if (ret == DHCP_FAILED) {
            dhcp_retry++;
            if (dhcp_retry <= 3) printf("task_network.c:\t DHCP Timeout occurred and retry [%d]\r\n", dhcp_retry);
        }
        if (dhcp_retry > 3) {
        	printf("task_network.c:\t DHCP Failed\r\n\r\n");
            DHCP_stop();
            break;
        }
    }

    // Schließe den DHCP-Socket nach der Verwendung
    close(SOCK_DHCP);
    return ret;
}

int8_t process_dns(void) {
    int8_t ret = 0;
    uint8_t dns_retry = 0;

    printf("task_network.c:\t DNS Client running\r\n");
    DNS_init(SOCK_DNS, data_buf);
    while (1) {
        if ((ret = DNS_run(dns_server, (uint8_t *)Domain_name, Domain_IP)) == 1) {
        	printf("task_network.c:\t DNS Success\r\n");
            break;
        } else {
            dns_retry++;
            if (dns_retry <= 2) printf("task_network.c:\t DNS Timeout occurred and retry [%d]\r\n", dns_retry);
        }
        if (dns_retry > 2) {
        	printf("task_network.c:\t DNS Failed\r\n\r\n");
            break;
        }
    }
    return ret;
}

void initialize_network(void) {
	printf("task_network.c:\t WizChip Init - \r\n");
    WIZCHIPInitialize();
    printf("task_network.c:\t version:%.2x\r\n", getVERSIONR());

    wizchip_setnetinfo(&defaultNetInfo);
    print_network_information();

    if (process_dhcp() == DHCP_IP_LEASED) {
        flag_process_dhcp_success = ON;
    } else {
        ctlnetwork(CN_SET_NETINFO, &defaultNetInfo); // Set default static IP settings
    }

    printf("task_network.c:\t Register value after W5x00 initialize!\r\n");
    print_network_information();

    if (process_dns()) {
        flag_process_dns_success = ON;
    }

    if (flag_process_dhcp_success == ENABLE) {
    	printf("task_network.c:\t DHCP IP Leased time : %lu seconds\r\n", getDHCPLeasetime());
    } else {
    	printf("task_network.c:\t DHCP Failed\r\n");
    }

    if (flag_process_dns_success == ENABLE) {
    	printf("task_network.c:\t DNS: %s => %d.%d.%d.%d\r\n", Domain_name, Domain_IP[0], Domain_IP[1], Domain_IP[2], Domain_IP[3]);
    } else {
    	printf("task_network.c:\t DNS Failed\r\n");
    }


}

int upgrade_to_websocket(uint8_t sn) {
    char request[] = "GET /chat HTTP/1.1\r\n"
                     "Host: example.com\r\n"
                     "Upgrade: websocket\r\n"
                     "Connection: Upgrade\r\n"
                     "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n"
                     "Sec-WebSocket-Version: 13\r\n\r\n";

    send(sn, (uint8_t*)request, strlen(request));

    uint8_t response[1024];
    int32_t len = recv(sn, response, sizeof(response) - 1); // Platz für Nullterminator lassen
    if (len <= 0) {
    	printf("task_network.c:\t Error receiving response.\r\n");
        return -1; // Fehler beim Empfangen der Antwort
    }

    response[len] = '\0';
    printf("task_network.c:\t Server response: %s\r\n", response);

    // Überprüfe die Antwort auf Erfolg
    if (strstr((char *)response, "HTTP/1.1 101 Switching Protocols") != NULL &&
        strstr((char *)response, "Upgrade: websocket") != NULL &&
        strstr((char *)response, "Connection: Upgrade") != NULL) {
    	printf("task_network.c:\t WebSocket upgrade successful.\r\n");
        return 0; // Erfolg
    } else {
    	printf("task_network.c:\t WebSocket upgrade failed.\r\n");
        return -1; // Fehler
    }
}




void StartNetworkTask(void *argument) {
    printf("task_network.c:\t StartwebSocketTask\r\n");

    printf("task_network.c:\t - initialize network\r\n");
    // configure network
    initialize_network();
    printf("task_network.c:\t - done\r\n");


    uint8_t *buf = (uint8_t *)malloc(DATA_BUF_SIZE);

    if (buf == NULL) {
        printf("task_network.c:\t Failed to allocate memory for buffer\n");
        return;
    }

    uint8_t destip[4] = {192, 168, 178, 25}; // Beispiel-IP-Adresse
    uint16_t destport = 8085; // port

    static uint16_t any_port = 50000;
    uint8_t currentSocketStatus = 0;

    int websocket_upgraded = 0;

    // Speicherzuteilung für die vollständige Nachricht
    int total_message_length = 0;
    char *total_message = (char *)malloc(DATA_BUF_SIZE);
    if (total_message == NULL) {
        printf("task_network.c:\t Failed to allocate memory for total_message buffer\n");
        free(buf);
        return;
    }
    total_message[0] = '\0'; // Initialisieren des total_message-Puffers

    for (;;) {
        currentSocketStatus = getSn_SR(SOCK_DHCP);
        switch (currentSocketStatus) {
            case SOCK_CLOSED:
                printf("task_network.c:\t %d: Socket closed, reopening...\r\n", SOCK_DHCP);
                if ((socket(SOCK_DHCP, Sn_MR_TCP, any_port++, 0x00)) != SOCK_DHCP) {
                    if (any_port == 0xffff) any_port = 50000;
                }
                printf("task_network.c:\t %d: Socket opened\r\n", SOCK_DHCP);
                break;

            case SOCK_INIT:
                printf("task_network.c:\t Socket is initialized.\r\n");
                printf("task_network.c:\t %d: Try to connect to the %d.%d.%d.%d : %d\r\n", SOCK_DHCP, destip[0], destip[1], destip[2], destip[3], destport);
                if (connect(SOCK_DHCP, destip, destport) != SOCK_OK) {
                    printf("task_network.c:\t PROBLEM\r\n");
                }
                break;

            case SOCK_ESTABLISHED:
                printf("task_network.c:\t Socket is established.\r\n");
                if (getSn_IR(SOCK_DHCP) & Sn_IR_CON) {
                    printf("task_network.c:\t %d: Connected to - %d.%d.%d.%d : %d\r\n", SOCK_DHCP, destip[0], destip[1], destip[2], destip[3], destport);
                    setSn_IR(SOCK_DHCP, Sn_IR_CON);
                }

                if (!websocket_upgraded) {
                    if (upgrade_to_websocket(SOCK_DHCP) == 0) {
                        printf("task_network.c:\t WebSocket upgrade successful.\n");
                        websocket_upgraded = 1;
                    } else {
                        printf("task_network.c:\t WebSocket upgrade failed.\n");
                        close(SOCK_DHCP);
                    }
                }

                int32_t ret;
                uint16_t size = 0;
                if ((size = getSn_RX_RSR(SOCK_DHCP)) > 0) {
                    if (size > DATA_BUF_SIZE - 1) size = DATA_BUF_SIZE - 1;
                    memset(buf, 0, DATA_BUF_SIZE);
                    ret = recv(SOCK_DHCP, buf, size);
                    if (ret <= 0) {
                        printf("task_network.c:\t Error receiving data. Socket closed.\n");
                        close(SOCK_DHCP);
                    } else {
                        buf[ret] = '\0';
                        //printf("task_network.c:\t Empfangene Nachricht: %s\r\n", buf);

                        // WebSocket-Frame analysieren
                        int header_length = 2;
                        uint64_t payload_length = buf[1] & 0x7F;
                        if (payload_length == 126) {
                            header_length = 4;
                            payload_length = (buf[2] << 8) + buf[3];
                        } else if (payload_length == 127) {
                            header_length = 10;
                            payload_length = ((uint64_t)buf[2] << 56) + ((uint64_t)buf[3] << 48) + ((uint64_t)buf[4] << 40) + ((uint64_t)buf[5] << 32) + (buf[6] << 24) + (buf[7] << 16) + (buf[8] << 8) + buf[9];
                        }

                        if (payload_length > DATA_BUF_SIZE - 1) {
                            printf("task_network.c:\t Received payload is too large\r\n");
                            break;
                        }

                        // Payload extrahieren
                        char *payload = (char *)(buf + header_length);

                        //printf("task_network.c:\t Bereinigte Nachricht: %.*s\r\n", (int)payload_length, payload);

                        // Payload zum total_message-Puffer hinzufügen
                        if (total_message_length + payload_length < DATA_BUF_SIZE) {
                            printf("task_network.c:\t Adding payload to total_message buffer. Current total_message_length: %d, payload_length: %d\r\n", total_message_length, (int)payload_length);
                            memcpy(total_message + total_message_length, payload, payload_length);
                            total_message_length += payload_length;
                            total_message[total_message_length] = '\0';
                            //printf("task_network.c:\t Updated total_message: %s\n", total_message);
                        } else {
                            printf("task_network.c:\t Buffer overflow, message too long.\n");
                            total_message_length = 0;
                            total_message[0] = '\0'; // Puffer zurücksetzen
                        }

                        // Überprüfen auf das Ende des Frames anhand des FIN-Bits
                        if ((buf[0] & 0x80) != 0) {  // FIN-Bit überprüfen
                            printf("task_network.c:\t Processing complete message\r\n");
                            process_received_data(total_message);
                            total_message_length = 0;
                            total_message[0] = '\0'; // Puffer zurücksetzen
                        }

                    }
                }
                break;

            default:
                printf("task_network.c:\t Unknown socket status: %d\n", currentSocketStatus);
                break;
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    // Puffer nach Beendigung der Schleife freigeben
    free(buf);
    free(total_message);
}

