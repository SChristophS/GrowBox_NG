/* task_network.c */

#include "task_network.h"
#include "cmsis_os.h"
#include "socket.h"
#include "wizchip_conf.h"
#include "wizchip_init.h"
#include "helper_websocket.h"
#include "controller_state.h"
#include "uart_redirect.h"
#include "globals.h"
#include "ds3231.h"
#include "eeprom.h"
#include "schedules.h"
#include "task_state_manager.h"
#include "cJSON.h"
#include "time_utils.h"
#include "sha1.h"
#include "base64.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* Netzwerk-Einstellungen */
#define MY_IP          {192, 168, 178, 100}
#define SUBNET_MASK    {255, 255, 255, 0}
#define GATEWAY        {192, 168, 178, 1}
#define DNS_SERVER     {8, 8, 8, 8}
#define MAC_ADDRESS    {0x00, 0x08, 0xdc, 0xab, 0xcd, 0xef}
#define LOCAL_PORT     50000

/* Zielserver-Einstellungen */
uint8_t destip[4] = {192, 168, 178, 25}; // IP-Adresse des Zielservers
uint16_t destport = 8085;                // Port des Zielservers

#define MAX_BUFFER_SIZE 2048

/* Funktionsprototypen */
void StartNetworkTask(void *argument);
void network_init(void);
void process_received_websocket_data(uint8_t sock, uint8_t *buf, int32_t size);
void process_received_data(const char *json_payload);
void parse_new_grow_cycle(cJSON *root);
void parse_led_schedules(cJSON *ledSchedules, GrowCycleConfig *config);
void parse_watering_schedules(cJSON *wateringSchedules, GrowCycleConfig *config);
bool websocket_handshake(uint8_t sock);
void process_websocket_messages(uint8_t sock);
void send_websocket_message(uint8_t sock, MessageForWebSocket *message);
void check_socket_status(uint8_t *socket_status, uint8_t sock, uint16_t *any_port, int *websocket_connected);

/* Globale Variablen */
uint8_t gDATABUF[MAX_BUFFER_SIZE];

wiz_NetInfo gWIZNETINFO = {
    .mac = MAC_ADDRESS,
    .ip = MY_IP,
    .sn = SUBNET_MASK,
    .gw = GATEWAY,
    .dns = DNS_SERVER,
    .dhcp = NETINFO_STATIC
};

void StartNetworkTask(void *argument)
{
    printf("task_network.c: Starting Network Task\r\n");

    /* Netzwerk initialisieren */
    network_init();

    uint8_t sock = 0;
    uint16_t any_port = LOCAL_PORT;
    int websocket_connected = 0;

    /* Socket initialisieren */
    if (socket(sock, Sn_MR_TCP, any_port++, 0x00) != sock) {
        if (any_port == 0xffff) any_port = 50000;
    }
    printf("task_network.c: Socket %d opened\r\n", sock);

    for (;;) {
        uint8_t socket_status = getSn_SR(sock);
        check_socket_status(&socket_status, sock, &any_port, &websocket_connected);

        int32_t ret;
        uint16_t size = 0;
        if ((size = getSn_RX_RSR(sock)) > 0) {
            if (size > MAX_BUFFER_SIZE - 1) size = MAX_BUFFER_SIZE - 1;
            memset(gDATABUF, 0, MAX_BUFFER_SIZE);
            ret = recv(sock, gDATABUF, size);
            if (ret <= 0) {
                printf("task_network.c: Error receiving data. Socket closed.\r\n");
                close(sock);
                websocket_connected = 0;
            } else {
                process_received_websocket_data(sock, gDATABUF, ret);
            }
        }

        /* Verarbeite ausgehende Nachrichten */
        if (websocket_connected) {
            process_websocket_messages(sock);
        }

        osDelay(100);
    }
}

void check_socket_status(uint8_t *socket_status, uint8_t sock, uint16_t *any_port, int *websocket_connected)
{
    switch (*socket_status) {
        case SOCK_CLOSED:
            printf("task_network.c: Socket %d closed, reopening...\r\n", sock);
            if ((socket(sock, Sn_MR_TCP, (*any_port)++, 0x00)) != sock) {
                if (*any_port == 0xffff) *any_port = 50000;
            }
            printf("task_network.c: Socket %d opened\r\n", sock);
            *websocket_connected = 0;
            break;

        case SOCK_INIT:
            printf("task_network.c: Socket %d is initialized.\r\n", sock);
            printf("task_network.c: Trying to connect to %d.%d.%d.%d:%d\r\n", destip[0], destip[1], destip[2], destip[3], destport);
            if (connect(sock, destip, destport) != SOCK_OK) {
                printf("task_network.c: Failed to connect to server\r\n");
            }
            break;

        case SOCK_ESTABLISHED:
            if (getSn_IR(sock) & Sn_IR_CON) {
                printf("task_network.c: Socket %d connected to %d.%d.%d.%d:%d\r\n", sock, destip[0], destip[1], destip[2], destip[3], destport);
                setSn_IR(sock, Sn_IR_CON);
            }

            if (!*websocket_connected) {
                /* WebSocket-Handshake durchführen */
                if (websocket_handshake(sock)) {
                    printf("task_network.c: WebSocket handshake successful\r\n");
                    *websocket_connected = 1;

                    // Registrierungsnachricht senden
					add_message_to_websocket_queue(MESSAGE_TYPE_REGISTER, DEVICE_CONTROLLER, 0, 0, 0);
                } else {
                    printf("task_network.c: WebSocket handshake failed\r\n");
                    close(sock);
                    *websocket_connected = 0;
                }
            }
            break;

        case SOCK_CLOSE_WAIT:
            printf("task_network.c: Socket %d close wait\r\n", sock);
            disconnect(sock);
            *websocket_connected = 0;
            break;

        default:
            break;
    }
}

void network_init(void)
{
    /* WIZCHIP initialisieren */
    uint8_t tmpstr[6];
    WIZCHIPInitialize();
    wizchip_setnetinfo(&gWIZNETINFO);
    wizchip_getnetinfo(&gWIZNETINFO);

    ctlwizchip(CW_GET_ID, (void*)tmpstr);
    printf("task_network.c: WIZCHIP Initialized with ID: %s\r\n", tmpstr);

    printf("task_network.c: MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
           gWIZNETINFO.mac[0], gWIZNETINFO.mac[1], gWIZNETINFO.mac[2],
           gWIZNETINFO.mac[3], gWIZNETINFO.mac[4], gWIZNETINFO.mac[5]);

    printf("task_network.c: IP Address: %d.%d.%d.%d\r\n",
           gWIZNETINFO.ip[0], gWIZNETINFO.ip[1], gWIZNETINFO.ip[2], gWIZNETINFO.ip[3]);

    printf("task_network.c: Subnet Mask: %d.%d.%d.%d\r\n",
           gWIZNETINFO.sn[0], gWIZNETINFO.sn[1], gWIZNETINFO.sn[2], gWIZNETINFO.sn[3]);

    printf("task_network.c: Gateway: %d.%d.%d.%d\r\n",
           gWIZNETINFO.gw[0], gWIZNETINFO.gw[1], gWIZNETINFO.gw[2], gWIZNETINFO.gw[3]);
}


bool websocket_handshake(uint8_t sock)
{
    printf("task_network.c: Performing WebSocket handshake\r\n");

    char request[] = "GET /chat HTTP/1.1\r\n"
                     "Host: example.com\r\n"
                     "Upgrade: websocket\r\n"
                     "Connection: Upgrade\r\n"
                     "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n"
                     "Sec-WebSocket-Version: 13\r\n\r\n";

    send(sock, (uint8_t *)request, strlen(request));

    /* Warten auf die Antwort */
    int32_t len;
    uint8_t response[MAX_BUFFER_SIZE];
    osDelay(500); // Warte etwas auf die Antwort
    if ((len = recv(sock, response, sizeof(response) - 1)) <= 0) {
        printf("task_network.c: Error receiving handshake response\r\n");
        return false;
    }

    response[len] = '\0';
    printf("task_network.c: Handshake response:\r\n%s\r\n", response);

    /* Überprüfen der Antwort */
    if (strstr((char *)response, "HTTP/1.1 101 Switching Protocols") != NULL &&
        strstr((char *)response, "Upgrade: websocket") != NULL &&
        strstr((char *)response, "Connection: Upgrade") != NULL) {
        printf("task_network.c: WebSocket handshake successful\r\n");
        return true;
    } else {
        printf("task_network.c: WebSocket handshake failed\r\n");
        return false;
    }
}

void process_websocket_messages(uint8_t sock)
{
    // Hier werden ausgehende Nachrichten verarbeitet
    // Zum Beispiel Nachrichten aus einer Queue senden

    MessageForWebSocket msg;

    while (osMessageQueueGet(xWebSocketQueueHandle, &msg, NULL, 0) == osOK) {

        printf("task_network.c: Folgende Nachricht wurde aus der Queue ausgelesen: message_type: %d, device: %d, target: %d, action: %d, value: %d\r\n",
                msg.message_type, msg.device, msg.target, msg.action, msg.value);

        printf("task_network.c: Weiterleiten an send_websocket_message \r\n");
        send_websocket_message(sock, &msg);
    }
}

void send_websocket_message(uint8_t sock, MessageForWebSocket *message)
{
    // Nachricht als JSON-String erstellen
    char json_message[256];

    snprintf(json_message, sizeof(json_message),
             "{\"UID\":\"%s\",\"message_type\":\"%s\",\"device\":\"%s\",\"target\":\"%s\",\"action\":\"%s\",\"value\":%u}",
             uidStr,
             message_type_to_string(message->message_type),
             device_to_string(message->device),
             target_to_string(message->target),
             action_to_string(message->action),
             message->value);

    size_t message_len = strlen(json_message);

    size_t frame_size;
    uint8_t *websocket_frame;

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

    // Nachrichtenstruktur serialisieren und maskieren
    for (size_t i = 0; i < message_len; i++) {
        websocket_frame[offset + i] = json_message[i] ^ masking_key[i % 4];
    }

    printf("task_network.c: Sending WebSocket frame with length: %lu\r\n", (unsigned long)frame_size);

    // Sende den Frame und prüfe auf Fehler
    int32_t total_sent = 0;
    while (total_sent < frame_size) {
        int32_t sent = send(sock, websocket_frame + total_sent, frame_size - total_sent);
        if (sent < 0) {
            printf("task_network.c: Failed to send WebSocket frame\r\n");
            free(websocket_frame);
            return;
        }
        total_sent += sent;
    }

    printf("task_network.c: Sent following JSON message:\r\n");
    printf("  JSON: %s\r\n", json_message);

    // Speicher freigeben
    free(websocket_frame);
}

void process_received_websocket_data(uint8_t sock, uint8_t *buf, int32_t size)
{
	printf("task_network.c: Received data of size %ld bytes\r\n", (long int)size);

    /* Annahme: buf enthält die empfangenen WebSocket-Daten */

    /* Parsing des WebSocket-Frames */
    uint8_t opcode = buf[0] & 0x0F;
    bool fin = (buf[0] & 0x80) != 0;
    bool mask = (buf[1] & 0x80) != 0;
    uint64_t payload_length = buf[1] & 0x7F;
    uint8_t header_length = 2;
    uint8_t masking_key[4] = {0};

    if (payload_length == 126) {
        payload_length = (buf[2] << 8) | buf[3];
        header_length += 2;
    } else if (payload_length == 127) {
        payload_length = ((uint64_t)buf[2] << 56) | ((uint64_t)buf[3] << 48) |
                         ((uint64_t)buf[4] << 40) | ((uint64_t)buf[5] << 32) |
                         ((uint64_t)buf[6] << 24) | ((uint64_t)buf[7] << 16) |
                         ((uint64_t)buf[8] << 8) | (uint64_t)buf[9];
        header_length += 8;
    }

    if (mask) {
        memcpy(masking_key, &buf[header_length], 4);
        header_length += 4;
    }

    printf("task_network.c: Opcode: %d, FIN: %d, Masked: %d, Payload length: %llu\r\n",
           opcode, fin, mask, payload_length);

    /* Unmasking des Payloads */
    if (mask) {
        for (uint64_t i = 0; i < payload_length; i++) {
            buf[header_length + i] ^= masking_key[i % 4];
        }
    }

    /* Sicherstellen, dass der Payload nullterminiert ist */
    buf[header_length + payload_length] = '\0';

    printf("task_network.c: Payload: %s\r\n", &buf[header_length]);

    /* Verarbeitung des Payloads */
    process_received_data((char *)&buf[header_length]);
}

void process_received_data(const char *json_payload)
{
    printf("task_network.c: Processing received JSON data\r\n");

    cJSON *root = cJSON_Parse(json_payload);
    if (root == NULL) {
        printf("task_network.c: JSON Parsing Error\r\n");
        return;
    }

    cJSON *message_type = cJSON_GetObjectItem(root, "message_type");
    if (message_type == NULL || !cJSON_IsString(message_type)) {
        printf("task_network.c: No message_type found\r\n");
        cJSON_Delete(root);
        return;
    }

    printf("task_network.c: Message type: %s\r\n", message_type->valuestring);

    // Überprüfe die target_UUID
    cJSON *target_UUID = cJSON_GetObjectItem(root, "target_UUID");
    if (target_UUID == NULL || !cJSON_IsString(target_UUID)) {
        printf("task_network.c: No target_UUID found\r\n");
        cJSON_Delete(root);
        return;
    }

    // Vergleiche target_UUID mit der Seriennummer des Controllers
    if (strcmp(target_UUID->valuestring, uidStr) != 0) {
        printf("task_network.c: target_UUID does not match this controller's UID\r\n");
        cJSON_Delete(root);
        return;
    }

    // Aktuelle Zeit synchronisieren, falls vorhanden
    cJSON *current_time = cJSON_GetObjectItem(root, "current_time");
    if (current_time != NULL && cJSON_IsString(current_time)) {
        printf("task_network.c: Synchronizing RTC with current_time: %s\r\n", current_time->valuestring);
        synchronize_rtc(current_time->valuestring);
    }

    if (strcmp(message_type->valuestring, "newGrowCycle") == 0) {
        parse_new_grow_cycle(root);
    } else {
        printf("task_network.c: Unknown message_type: %s\r\n", message_type->valuestring);
    }

    cJSON_Delete(root);
}

void parse_new_grow_cycle(cJSON *root)
{
    printf("task_network.c: Parsing new grow cycle configuration\r\n");

    cJSON *payload = cJSON_GetObjectItem(root, "payload");
    if (payload == NULL) {
        printf("task_network.c: No payload found in newGrowCycle message\r\n");
        return;
    }

    cJSON *value = cJSON_GetObjectItem(payload, "value");
    if (value == NULL) {
        printf("task_network.c: No value found in payload\r\n");
        return;
    }

    GrowCycleConfig newConfig;
    memset(&newConfig, 0, sizeof(GrowCycleConfig));

    /* startGrowTime */
    cJSON *startGrowTime = cJSON_GetObjectItem(value, "startGrowTime");
    if (startGrowTime != NULL && cJSON_IsString(startGrowTime)) {
        printf("task_network.c: startGrowTime: %s\r\n", startGrowTime->valuestring);
        synchronize_rtc(startGrowTime->valuestring);
    } else {
        printf("task_network.c: startGrowTime not found or invalid\r\n");
    }

    /* ledSchedules */
    cJSON *ledSchedules = cJSON_GetObjectItem(value, "ledSchedules");
    if (ledSchedules != NULL && cJSON_IsArray(ledSchedules)) {
        parse_led_schedules(ledSchedules, &newConfig);
    } else {
        printf("task_network.c: ledSchedules not found or invalid\r\n");
    }

    /* wateringSchedules */
    cJSON *wateringSchedules = cJSON_GetObjectItem(value, "wateringSchedules");
    if (wateringSchedules != NULL && cJSON_IsArray(wateringSchedules)) {
        parse_watering_schedules(wateringSchedules, &newConfig);
    } else {
        printf("task_network.c: wateringSchedules not found or invalid\r\n");
    }

    /* Weitere Zeitpläne können ähnlich geparst werden */

    /* Speichern der neuen Konfiguration im EEPROM */
    if (save_grow_cycle_config(&newConfig)) {
        printf("task_network.c: Grow cycle configuration saved successfully\r\n");
    } else {
        printf("task_network.c: Failed to save grow cycle configuration\r\n");
    }
}

void parse_led_schedules(cJSON *ledSchedules, GrowCycleConfig *config)
{
    printf("task_network.c: Parsing LED schedules\r\n");

    int scheduleCount = cJSON_GetArraySize(ledSchedules);
    for (int i = 0; i < scheduleCount && i < MAX_LED_SCHEDULES; i++) {
        cJSON *schedule = cJSON_GetArrayItem(ledSchedules, i);
        if (schedule != NULL) {
            cJSON *durationOn = cJSON_GetObjectItem(schedule, "durationOn");
            cJSON *durationOff = cJSON_GetObjectItem(schedule, "durationOff");
            cJSON *repetition = cJSON_GetObjectItem(schedule, "repetition");

            if (durationOn && durationOff && repetition) {
                config->ledSchedules[i].durationOn = durationOn->valueint;
                config->ledSchedules[i].durationOff = durationOff->valueint;
                config->ledSchedules[i].repetition = repetition->valueint;
                config->ledScheduleCount++;
                printf("task_network.c: Added LED schedule %d: durationOn=%lu, durationOff=%lu, repetition=%d\r\n",
                       i, (unsigned long)config->ledSchedules[i].durationOn, (unsigned long)config->ledSchedules[i].durationOff,
                       config->ledSchedules[i].repetition);
            } else {
                printf("task_network.c: Incomplete ledSchedule data\r\n");
            }
        }
    }
}

void parse_watering_schedules(cJSON *wateringSchedules, GrowCycleConfig *config)
{
    printf("task_network.c: Parsing watering schedules\r\n");

    int scheduleCount = cJSON_GetArraySize(wateringSchedules);
    for (int i = 0; i < scheduleCount && i < MAX_WATERING_SCHEDULES; i++) {
        cJSON *schedule = cJSON_GetArrayItem(wateringSchedules, i);
        if (schedule != NULL) {
            cJSON *status1 = cJSON_GetObjectItem(schedule, "status1");
            cJSON *duration1 = cJSON_GetObjectItem(schedule, "duration1");
            cJSON *status2 = cJSON_GetObjectItem(schedule, "status2");
            cJSON *duration2 = cJSON_GetObjectItem(schedule, "duration2");
            cJSON *waterRepetitions = cJSON_GetObjectItem(schedule, "waterRepetitions");

            if (status1 && duration1 && status2 && duration2 && waterRepetitions) {
                // Hier solltest du die entsprechenden Felder in deiner Struktur zuweisen
                // Beispiel:
                strncpy(config->wateringSchedules[i].status1, status1->valuestring, sizeof(config->wateringSchedules[i].status1)-1);
                config->wateringSchedules[i].status1[sizeof(config->wateringSchedules[i].status1)-1] = '\0';
                config->wateringSchedules[i].duration1 = duration1->valueint;
                // ... und so weiter
                config->wateringScheduleCount++;
                printf("task_network.c: Added watering schedule %d\r\n", i);
            } else {
                printf("task_network.c: Incomplete wateringSchedule data\r\n");
            }
        }
    }
}


void synchronize_rtc(const char *iso8601_time)
{
    struct tm tm_time;
    memset(&tm_time, 0, sizeof(struct tm));

    if (strptime(iso8601_time, "%Y-%m-%dT%H:%M:%S", &tm_time) == NULL) {
        printf("task_network.c: Failed to parse current_time\r\n");
        return;
    }

    DS3231_Time newTime;
    newTime.year = tm_time.tm_year + 1900;
    newTime.month = tm_time.tm_mon + 1;
    newTime.dayOfMonth = tm_time.tm_mday;
    newTime.hours = tm_time.tm_hour;
    newTime.minutes = tm_time.tm_min;
    newTime.seconds = tm_time.tm_sec;

    if (DS3231_SetTime(&newTime)) {
        printf("task_network.c: RTC time updated successfully\r\n");
    } else {
        printf("task_network.c: Failed to update RTC time\r\n");
    }
}
