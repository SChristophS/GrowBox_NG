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
#include <stdlib.h>
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
void parse_control_command(cJSON *root);
void parse_new_grow_cycle(cJSON *root);
void parse_led_schedules(cJSON *ledSchedules, GrowCycleConfig *config);
void parse_watering_schedules(cJSON *wateringSchedules, GrowCycleConfig *config);
bool websocket_handshake(uint8_t sock);
void process_websocket_messages(uint8_t sock);
void send_websocket_message(uint8_t sock, MessageForWebSocket *message);
void check_socket_status(uint8_t *socket_status, uint8_t sock, uint16_t *any_port, int *websocket_connected);
void synchronize_rtc(const char *iso8601_time);
void handle_water_command(const char *action, cJSON *value);
void handle_system_command(const char *action, cJSON *value);
uint32_t calculate_total_duration(GrowCycleConfig *config);
void handle_light_command(const char *action, cJSON *value);
void handle_pump_command(const char *action, int deviceId, cJSON *value);
bool parse_iso8601_datetime(const char *datetime_str, struct tm *tm_time);
time_t ds3231_time_to_timestamp(DS3231_Time *time);

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

    printf("task_network.c: calling InitializeGrowCycleConfig\r\n");
    InitializeGrowCycleConfig();


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
    } else if (strcmp(message_type->valuestring, "ControlCommand") == 0) {
        parse_control_command(root);
    } else {
        printf("task_network.c: Unknown message_type: %s\r\n", message_type->valuestring);
    }

    cJSON_Delete(root);
}

void parse_control_command(cJSON *root)
{
    printf("task_network.c: Parsing ControlCommand\r\n");

    cJSON *payload = cJSON_GetObjectItem(root, "payload");
    if (payload == NULL) {
        printf("task_network.c: No payload found in ControlCommand message\r\n");
        return;
    }

    cJSON *commands = cJSON_GetObjectItem(payload, "commands");
    if (commands == NULL || !cJSON_IsArray(commands)) {
        printf("task_network.c: No commands array found in payload\r\n");
        return;
    }

    int commandCount = cJSON_GetArraySize(commands);
    for (int i = 0; i < commandCount; i++) {
        cJSON *command = cJSON_GetArrayItem(commands, i);
        if (command != NULL) {
            cJSON *target = cJSON_GetObjectItem(command, "target");
            cJSON *action = cJSON_GetObjectItem(command, "action");
            cJSON *value = cJSON_GetObjectItem(command, "value");

            if (target && action && value && cJSON_IsString(target) && cJSON_IsString(action)) {
                if (strcmp(target->valuestring, "light") == 0) {
                    handle_light_command(action->valuestring, value);
                } else if (strcmp(target->valuestring, "pump") == 0) {
                    cJSON *deviceId = cJSON_GetObjectItem(command, "deviceId");
                    if (deviceId && cJSON_IsNumber(deviceId)) {
                        handle_pump_command(action->valuestring, deviceId->valueint, value);
                    } else {
                        printf("task_network.c: pump command missing deviceId\r\n");
                    }
                } else if (strcmp(target->valuestring, "system") == 0) {
                    handle_system_command(action->valuestring, value);
                } else if (strcmp(target->valuestring, "water") == 0) {
                    handle_water_command(action->valuestring, value);
                } else {
                    printf("task_network.c: Unknown target: %s\r\n", target->valuestring);
                }
            } else {
                printf("task_network.c: Invalid command format\r\n");
            }
        }
    }
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
        strncpy(newConfig.startGrowTime, startGrowTime->valuestring, sizeof(newConfig.startGrowTime));
    } else {
        printf("task_network.c: startGrowTime not found or invalid\r\n");
        return;
    }

    /* automaticMode */
    cJSON *automaticMode = cJSON_GetObjectItem(value, "automaticMode");
    if (automaticMode != NULL && cJSON_IsBool(automaticMode)) {
        newConfig.automaticMode = cJSON_IsTrue(automaticMode);
        printf("task_network.c: automaticMode: %s\r\n", newConfig.automaticMode ? "true" : "false");
    } else {
        printf("task_network.c: automaticMode not found or invalid, defaulting to false\r\n");
        newConfig.automaticMode = false;
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

    // Gültigkeitsprüfung des GrowPlans
    uint32_t totalDuration = calculate_total_duration(&newConfig);
    printf("task_network.c: totalDuration = %li\r\n", totalDuration);

    // Aktuelle Zeit holen
    DS3231_Time currentTime;
    if (!DS3231_GetTime(&currentTime)) {
        printf("task_network.c: Failed to get current time from RTC\r\n");
        return;
    }

    // Startzeit parsen
    struct tm tm_start;
    if (!parse_iso8601_datetime(newConfig.startGrowTime, &tm_start)) {
        printf("task_network.c: Failed to parse startGrowTime\r\n");
        return;
    }

    // Umwandlung in Zeitstempel
    time_t startTimestamp = mktime(&tm_start);
    time_t currentTimestamp = ds3231_time_to_timestamp(&currentTime);

    // Prüfen, ob der GrowPlan abgelaufen ist
    if ((currentTimestamp - startTimestamp) > totalDuration) {
        printf("task_network.c: GrowPlan has already expired, not saving configuration\r\n");
        return;
    } else {
    	printf("task_network.c: Grow is still valid\r\n");
    }



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
            cJSON *duration_full = cJSON_GetObjectItem(schedule, "duration_full");
            cJSON *duration_empty = cJSON_GetObjectItem(schedule, "duration_empty");
            cJSON *repetition = cJSON_GetObjectItem(schedule, "repetition");

            if (duration_full && duration_empty && repetition) {
                // Werte zuweisen
                config->wateringSchedules[i].duration_full = duration_full->valueint;
                config->wateringSchedules[i].duration_empty = duration_empty->valueint;
                config->wateringSchedules[i].repetition = repetition->valueint;
                config->wateringScheduleCount++;
                printf("task_network.c: Added watering schedule %d: duration_full=%lu, duration_empty=%lu, repetition=%d\r\n",
                       i, (unsigned long)config->wateringSchedules[i].duration_full, (unsigned long)config->wateringSchedules[i].duration_empty,
                       config->wateringSchedules[i].repetition);

            } else {
                printf("task_network.c: Incomplete wateringSchedule data\r\n");
            }
        }
    }
}

bool parse_iso8601_datetime(const char *datetime_str, struct tm *tm_time)
{
    int year, month, day, hour, min, sec;
    if (sscanf(datetime_str, "%4d-%2d-%2dT%2d:%2d:%2d",
               &year, &month, &day, &hour, &min, &sec) != 6)
    {
        return false;
    }

    tm_time->tm_year = year - 1900; // `tm_year` ist die Anzahl der Jahre seit 1900
    tm_time->tm_mon = month - 1;    // `tm_mon` ist 0-basiert (0 = Januar)
    tm_time->tm_mday = day;
    tm_time->tm_hour = hour;
    tm_time->tm_min = min;
    tm_time->tm_sec = sec;
    tm_time->tm_isdst = -1; // Daylight Saving Time unbekannt

    return true;
}

void synchronize_rtc(const char *iso8601_time)
{
    struct tm tm_time;
    memset(&tm_time, 0, sizeof(struct tm));

    if (!parse_iso8601_datetime(iso8601_time, &tm_time)) {
        printf("task_network.c: Failed to parse current_time\n");
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

void handle_water_command(const char *action, cJSON *value)
{
    if (strcmp(action, "setState") == 0 && cJSON_IsString(value)) {
        const char *desiredState = value->valuestring;
        printf("task_network.c: Forwarding water state command: %s\r\n", desiredState);

        // Sende den gewünschten Zustand an die Wassersteuerungs-Task
        WaterCommand waterCmd;
        if (strcmp(desiredState, "full") == 0) {
            waterCmd.commandType = WATER_COMMAND_SET_STATE;
            waterCmd.desiredState = WATER_STATE_FULL;
        } else if (strcmp(desiredState, "empty") == 0) {
            waterCmd.commandType = WATER_COMMAND_SET_STATE;
            waterCmd.desiredState = WATER_STATE_EMPTY;
        } else {
            printf("task_network.c: Unknown water state: %s\r\n", desiredState);
            return;
        }

        // Sende den Befehl über eine Message Queue
        if (osMessageQueuePut(xWaterCommandQueueHandle, &waterCmd, 0, 0) != osOK) {
            printf("task_network.c: Failed to send water command to water controller\r\n");
        }
        printf("task_network.c: finished send water command to water controller\r\n");
    } else {
        printf("task_network.c: Unknown action for water: %s\r\n", action);
    }
}

void handle_light_command(const char *action, cJSON *value)
{
    if (strcmp(action, "setIntensity") == 0 && cJSON_IsNumber(value)) {
        uint8_t intensity = (uint8_t)value->valueint;
        printf("task_network.c: Forwarding light intensity command: %d\r\n", intensity);

        // Sende den gewünschten Intensitätswert an die Lichtsteuerungs-Task
        LightCommand lightCmd;
        lightCmd.commandType = LIGHT_COMMAND_SET_INTENSITY;
        lightCmd.intensity = intensity;

        // Sende den Befehl über eine Message Queue
        if (osMessageQueuePut(xLightCommandQueueHandle, &lightCmd, 0, 0) != osOK) {
            printf("task_network.c: Failed to send light command to light controller\r\n");
        }
        printf("task_network.c: Finished sending light command to light controller\r\n");
    } else {
        printf("task_network.c: Unknown action for light: %s\r\n", action);
    }
}


void handle_pump_command(const char *action, int deviceId, cJSON *value)
{
    if (strcmp(action, "setState") == 0 && cJSON_IsBool(value)) {
        bool enable = cJSON_IsTrue(value);
        printf("task_network.c: Setting pump %d state to %s\r\n", deviceId, enable ? "ON" : "OFF");

        // Aktualisiere den Controller-Zustand
        UpdatePumpState(enable, deviceId);

        // Sende Befehl an Hardware-Task
        ControlPump(enable, deviceId);
    } else {
        printf("task_network.c: Unknown action for pump: %s\r\n", action);
    }
}


void handle_system_command(const char *action, cJSON *value)
{
    if (strcmp(action, "setAutomaticMode") == 0 && cJSON_IsBool(value)) {
        bool automaticMode = cJSON_IsTrue(value);
        printf("task_network.c: Setting automatic mode to %s\r\n", automaticMode ? "ON" : "OFF");

        // Aktualisiere die globale Konfiguration
        osMutexAcquire(gGrowCycleConfigMutexHandle, osWaitForever);
        gGrowCycleConfig.automaticMode = automaticMode;
        osMutexRelease(gGrowCycleConfigMutexHandle);

        // Speichere nur den automaticMode im EEPROM
        save_automatic_mode(automaticMode);

        // Event-Flag setzen, um die Controller-Tasks zu benachrichtigen
        osEventFlagsSet(gControllerEventGroupHandle, NEW_GROW_CYCLE_CONFIG_AVAILABLE);
    } else {
        printf("task_network.c: Unknown action for system: %s\r\n", action);
    }
}


uint32_t calculate_total_duration(GrowCycleConfig *config) {
    uint32_t totalDuration = 0;
    // Berechnung der Dauer der LED-Zeitpläne
    for (uint8_t i = 0; i < config->ledScheduleCount; i++) {
        LedSchedule *schedule = &config->ledSchedules[i];
        uint32_t scheduleDuration = (schedule->durationOn + schedule->durationOff) * schedule->repetition;
        totalDuration += scheduleDuration;
    }
    // Berechnung der Dauer der Bewässerungszeitpläne (optional, wenn sie separat berücksichtigt werden sollen)
    // ...
    return totalDuration; // Dauer in Sekunden
}
