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
    .mac = {0x00, 0x08, 0xdc, 0xff, 0xee, 0xdd},
    .ip = {192, 168, 178, 100},
    .sn = {255, 255, 255, 0},
    .gw = {192, 168, 178, 1},
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
    printf("task_network.c:\t Mac address: %02x:%02x:%02x:%02x:%02x:%02x\r\n", defaultNetInfo.mac[0], defaultNetInfo.mac[1], defaultNetInfo.mac[2], defaultNetInfo.mac[3], defaultNetInfo.mac[4], defaultNetInfo.mac[5]);
    printf("task_network.c:\t IP address : %d.%d.%d.%d\r\n", defaultNetInfo.ip[0], defaultNetInfo.ip[1], defaultNetInfo.ip[2], defaultNetInfo.ip[3]);
    printf("task_network.c:\t SM Mask    : %d.%d.%d.%d\r\n", defaultNetInfo.sn[0], defaultNetInfo.sn[1], defaultNetInfo.sn[2], defaultNetInfo.sn[3]);
    printf("task_network.c:\t Gate way   : %d.%d.%d.%d\r\n", defaultNetInfo.gw[0], defaultNetInfo.gw[1], defaultNetInfo.gw[2], defaultNetInfo.gw[3]);
    printf("task_network.c:\t DNS Server : %d.%d.%d.%d\r\n", defaultNetInfo.dns[0], defaultNetInfo.dns[1], defaultNetInfo.dns[2], defaultNetInfo.dns[3]);
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




void send_websocket_message(uint8_t sock, MessageForWebSocket msg) {
    // Konvertiere die Struktur in eine JSON-ähnliche Zeichenkette
    char message[256];
    snprintf(message, sizeof(message), "{\"target\": %u, \"value\": %s}", msg.target, msg.value ? "true" : "false");

    size_t message_len = strlen(message);
    size_t frame_size;
    uint8_t *websocket_frame;

    printf("task_network.c: Preparing to send message: %s\r\n", message);

    // Berechnen der Frame-Größe basierend auf der Nachrichtengröße
    if (message_len <= 125) {
        frame_size = message_len + 6; // 2 Byte Header + 4 Byte Masking Key
    } else if (message_len <= 65535) {
        frame_size = message_len + 8; // 4 Byte Extended Payload + 2 Byte Header + 4 Byte Masking Key
    } else {
        printf("task_network.c: Message too long to be sent in a single WebSocket frame\r\n");
        return;
    }

    // Speicher für den Frame allokieren
    websocket_frame = (uint8_t *)malloc(frame_size);
    if (websocket_frame == NULL) {
        printf("task_network.c: Failed to allocate memory for WebSocket frame\r\n");
        return;
    }

    // WebSocket-Frame-Header erstellen
    websocket_frame[0] = 0x81; // FIN-Bit gesetzt, Text-Frame

    size_t offset;
    if (message_len <= 125) {
        websocket_frame[1] = 0x80 | message_len; // Mask-Bit gesetzt, Payload-Länge
        offset = 2;
    } else if (message_len <= 65535) {
        websocket_frame[1] = 0x80 | 126; // Mask-Bit gesetzt, Extended Payload-Länge
        websocket_frame[2] = (message_len >> 8) & 0xFF;
        websocket_frame[3] = message_len & 0xFF;
        offset = 4;
    }

    // Generiere Masking Key
    uint8_t masking_key[4];
    for (int i = 0; i < 4; i++) {
        masking_key[i] = rand() % 256;
    }
    memcpy(&websocket_frame[offset], masking_key, 4);
    offset += 4;

    // Anwenden der Maskierung auf die Payload
    for (size_t i = 0; i < message_len; i++) {
        websocket_frame[offset + i] = message[i] ^ masking_key[i % 4];
    }

    printf("task_network.c: Sending WebSocket frame with length: %zu\r\n", frame_size);

    // Sende den Frame und prüfe auf Fehler
    int32_t total_sent = 0;
    while (total_sent < frame_size) {
        int32_t sent = send(sock, websocket_frame + total_sent, frame_size - total_sent);
        if (sent < 0) {
            perror("task_network.c: Failed to send WebSocket frame");
            free(websocket_frame);
            return;
        }
        total_sent += sent;
    }

    printf("task_network.c: Sent message: %s\r\n", message);

    // Speicher freigeben
    free(websocket_frame);
}





void process_received_websocket_data(uint8_t sock, uint8_t *buf, int32_t size) {
    buf[size] = '\0';
    printf("task_network.c:\t Empfangene Nachricht: %s\r\n", buf);

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

    printf("task_network.c:\t Payload length: %llu\r\n", payload_length);

    if (payload_length > DATA_BUF_SIZE - 1) {
        printf("task_network.c:\t Received payload is too large\r\n");
        return;
    }

    // Payload extrahieren
    char *payload = (char *)(buf + header_length);
    printf("task_network.c:\t Bereinigte Nachricht: %.*s\r\n", (int)payload_length, payload);

    // Payload verarbeiten
    process_received_data(payload);
}

void check_socket_status(uint8_t *socket_status, uint8_t sock, uint16_t *any_port, uint8_t destip[4], uint16_t destport, int *websocket_upgraded) {
    switch (*socket_status) {
        case SOCK_CLOSED:
            printf("task_network.c:\t %d: Socket closed, reopening...\r\n", sock);
            if ((socket(sock, Sn_MR_TCP, (*any_port)++, 0x00)) != sock) {
                if (*any_port == 0xffff) *any_port = 50000;
            }
            printf("task_network.c:\t %d: Socket opened\r\n", sock);
            break;

        case SOCK_INIT:
            printf("task_network.c:\t Socket is initialized.\r\n");
            printf("task_network.c:\t %d: Try to connect to the %d.%d.%d.%d : %d\r\n", sock, destip[0], destip[1], destip[2], destip[3], destport);
            if (connect(sock, destip, destport) != SOCK_OK) {
                printf("task_network.c:\t PROBLEM\r\n");
            }
            break;

        case SOCK_ESTABLISHED:
            printf("task_network.c:\t Socket is established.\r\n");
            if (getSn_IR(sock) & Sn_IR_CON) {
                printf("task_network.c:\t %d: Connected to - %d.%d.%d.%d : %d\r\n", sock, destip[0], destip[1], destip[2], destip[3], destport);
                setSn_IR(sock, Sn_IR_CON);
            }

            if (!*websocket_upgraded) {
                if (upgrade_to_websocket(sock) == 0) {
                    printf("task_network.c:\t WebSocket upgrade successful.\r\n");
                    *websocket_upgraded = 1;
                } else {
                    printf("task_network.c:\t WebSocket upgrade failed.\r\n");
                    close(sock);
                }
            }
            break;

        default:
            printf("task_network.c:\t Unknown socket status: %d\r\n", *socket_status);
            break;
    }
}

void StartNetworkTask(void *argument) {
    printf("task_network.c: StartwebSocketTask\r\n");

    printf("task_network.c: - initialize network\r\n");
    initialize_network();
    printf("task_network.c: - done\r\n");

    uint8_t *buf = (uint8_t *)malloc(DATA_BUF_SIZE);
    if (buf == NULL) {
        printf("task_network.c: Failed to allocate memory for buffer\r\n");
        return;
    }

    uint8_t destip[4] = {192, 168, 178, 25}; // Beispiel-IP-Adresse
    uint16_t destport = 8085; // port
    static uint16_t any_port = 50000;
    uint8_t currentSocketStatus = 0;
    int websocket_upgraded = 0;
    char *total_message = (char *)malloc(DATA_BUF_SIZE);
    if (total_message == NULL) {
        printf("task_network.c: Failed to allocate memory for total_message buffer\r\n");
        free(buf);
        return;
    }
    total_message[0] = '\0'; // Initialisieren des total_message-Puffers

    for (;;) {
        currentSocketStatus = getSn_SR(SOCK_DHCP);
        check_socket_status(&currentSocketStatus, SOCK_DHCP, &any_port, destip, destport, &websocket_upgraded);

        int32_t ret;
        uint16_t size = 0;
        if ((size = getSn_RX_RSR(SOCK_DHCP)) > 0) {
            if (size > DATA_BUF_SIZE - 1) size = DATA_BUF_SIZE - 1;
            memset(buf, 0, DATA_BUF_SIZE);
            ret = recv(SOCK_DHCP, buf, size);
            if (ret <= 0) {
                printf("task_network.c: Error receiving data. Socket closed.\r\n");
                close(SOCK_DHCP);
            } else {
                process_received_websocket_data(SOCK_DHCP, buf, ret);
            }
        }


        MessageForWebSocket msg;
        // Nachrichten aus der WebSocket-Queue senden
        while (osMessageQueueGet(xWebSocketQueueHandle, &msg, NULL, 0) == osOK) {
            printf("task_network.c: Struct msg - target: %u, value: %d\r\n", msg.target, msg.value);
            send_websocket_message(SOCK_DHCP, msg);
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    // Puffer nach Beendigung der Schleife freigeben
    free(buf);
    free(total_message);
}
