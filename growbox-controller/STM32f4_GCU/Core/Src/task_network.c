/* task_network.c */

#include "task_network.h"
#include "cmsis_os.h"
#include "socket.h"
#include "wizchip_conf.h"
#include "wizchip_init.h"
#include "helper_websocket.h"
#include "controller_state.h"
#include "logger.h"
#include "globals.h"

#include "state_manager.h"
#include "ds3231.h"
#include "eeprom.h"

#include "cJSON.h"
#include "time_utils.h"
#include "sha1.h"
#include "base64.h"
#include "task_water_controller.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include "message_types.h"

#define EEPROM_SIZE 32768 // Größe des AT24C256 in Bytes

/* Funktionsprototypen */

/* Hauptfunktion der Netzwerkaufgabe */
void StartNetworkTask(void *argument);

/* Initialisiert das Netzwerk */
void network_init(void);

/* Überprüft den Socket-Status und führt entsprechende Aktionen aus */
void check_socket_status(uint8_t *socket_status, uint8_t sock, uint16_t *any_port, int *websocket_connected);

/* Führt den WebSocket-Handschlag durch */
bool websocket_handshake(uint8_t sock);

/* Verarbeitet ausgehende WebSocket-Nachrichten */
void process_websocket_messages(uint8_t sock);

/* Sendet eine WebSocket-Nachricht */
void send_websocket_message(uint8_t sock, MessageForWebSocket *message);

/* Verarbeitet empfangene WebSocket-Daten */
void process_received_websocket_data(uint8_t sock, uint8_t *buf, int32_t size);

/* Verarbeitet empfangene JSON-Daten */
void process_received_data(const char *json_payload);

/* Handhabt eine TimeSync-Nachricht */
void handle_timesync_message(cJSON *root);

/* Handhabt eine StatusRequest-Nachricht */
void handle_status_request(cJSON *root);

/* Handhabt eine EraseEEPROM-Nachricht */
void handle_erase_eeprom_message(void);

/* Sendet die GrowCycle-Konfiguration */
void send_grow_cycle_config(void);

/* Sendet den Controller-Zustand */
void send_controller_state(void);

/* Konvertiert die GrowCycle-Konfiguration in JSON */
cJSON *grow_cycle_config_to_json(GrowCycleConfig *config);

/* Konvertiert den Controller-Zustand in JSON */
cJSON *controller_state_to_json(ControllerState *state);

/* Sendet JSON über WebSocket */
void send_json_over_websocket(const char *json_string);

/* Parsen von ControlCommand-Nachrichten */
void parse_control_command(cJSON *root);

/* Handhabt Lichtbefehle */
void handle_light_command(const char *action, cJSON *value);

/* Handhabt Pumpenbefehle */
void handle_pump_command(const char *action, int deviceId, cJSON *value);

/* Handhabt Systembefehle */
void handle_system_command(const char *action, cJSON *value);

/* Handhabt Wasserbefehle */
void handle_water_command(const char *action, cJSON *value);

/* Parsen von neuen GrowCycle-Nachrichten */
void parse_new_grow_cycle(cJSON *root);

/* Parsen von LED-Zeitplänen */
void parse_led_schedules(cJSON *ledSchedules, GrowCycleConfig *config);

/* Parsen von Bewässerungszeitplänen */
void parse_watering_schedules(cJSON *wateringSchedules, GrowCycleConfig *config);

/* Synchronisiert die RTC */
void synchronize_rtc(const char *iso8601_time);

/* Berechnet die Gesamtdauer des GrowCycle */
uint32_t calculate_total_duration(GrowCycleConfig *config);



/* Zielserver-Einstellungen */
uint8_t destip[4] = {192, 168, 178, 25}; // IP-Adresse des Zielservers
uint16_t destport = 8085;                // Port des Zielservers

#define MAX_BUFFER_SIZE 2048



/* Globale Variablen */
uint8_t DATABUF[MAX_BUFFER_SIZE];

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
    LOG_INFO("task_network.c: Starting Network Task");

    /* Netzwerk initialisieren */
    network_init();

    uint8_t sock = 0;
    uint16_t any_port = LOCAL_PORT;
    int websocket_connected = 0;

    /* Socket initialisieren */
    if (socket(sock, Sn_MR_TCP, any_port++, 0x00) != sock) {
        if (any_port == 0xffff) any_port = 50000;
    }
    LOG_INFO("task_network.c: Socket %d opened", sock);

    for (;;) {
        uint8_t socket_status = getSn_SR(sock);
        check_socket_status(&socket_status, sock, &any_port, &websocket_connected);

        int32_t ret;
        uint16_t size = 0;
        if ((size = getSn_RX_RSR(sock)) > 0) {
            if (size > MAX_BUFFER_SIZE - 1) size = MAX_BUFFER_SIZE - 1;
            memset(DATABUF, 0, MAX_BUFFER_SIZE);
            ret = recv(sock, DATABUF, size);
            if (ret <= 0) {
                LOG_ERROR("task_network.c: Error receiving data. Socket closed.");
                close(sock);
                websocket_connected = 0;
            } else {
                process_received_websocket_data(sock, DATABUF, ret);
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
            LOG_INFO("task_network.c: Socket %d closed, reopening...", sock);
            if ((socket(sock, Sn_MR_TCP, (*any_port)++, 0x00)) != sock) {
                if (*any_port == 0xffff) *any_port = 50000;
            }
            LOG_INFO("task_network.c: Socket %d opened", sock);
            *websocket_connected = 0;
            break;

        case SOCK_INIT:
            LOG_INFO("task_network.c: Socket %d is initialized.", sock);
            LOG_INFO("task_network.c: Trying to connect to %d.%d.%d.%d:%d",
                     destip[0], destip[1], destip[2], destip[3], destport);
            if (connect(sock, destip, destport) != SOCK_OK) {
                LOG_ERROR("task_network.c: Failed to connect to server");
            }
            break;

        case SOCK_ESTABLISHED:
            if (getSn_IR(sock) & Sn_IR_CON) {
                LOG_INFO("task_network.c: Socket %d connected to %d.%d.%d.%d:%d",
                         sock, destip[0], destip[1], destip[2], destip[3], destport);
                setSn_IR(sock, Sn_IR_CON);
            }

            if (!*websocket_connected) {
                /* WebSocket-Handshake durchführen */
                if (websocket_handshake(sock)) {
                    LOG_INFO("task_network.c: WebSocket handshake successful");
                    *websocket_connected = 1;

                    // Registrierungsnachricht senden
                    add_message_to_websocket_queue(MESSAGE_TYPE_REGISTER, DEVICE_CONTROLLER, 0, 0, 0);
                } else {
                    LOG_ERROR("task_network.c: WebSocket handshake failed");
                    close(sock);
                    *websocket_connected = 0;
                }
            }
            break;

        case SOCK_CLOSE_WAIT:
            LOG_INFO("task_network.c: Socket %d close wait", sock);
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
    LOG_INFO("task_network.c: WIZCHIP Initialized with ID: %s", tmpstr);

    LOG_INFO("task_network.c: MAC Address: %02X:%02X:%02X:%02X:%02X:%02X",
             gWIZNETINFO.mac[0], gWIZNETINFO.mac[1], gWIZNETINFO.mac[2],
             gWIZNETINFO.mac[3], gWIZNETINFO.mac[4], gWIZNETINFO.mac[5]);

    LOG_INFO("task_network.c: IP Address: %d.%d.%d.%d",
             gWIZNETINFO.ip[0], gWIZNETINFO.ip[1], gWIZNETINFO.ip[2], gWIZNETINFO.ip[3]);

    LOG_INFO("task_network.c: Subnet Mask: %d.%d.%d.%d",
             gWIZNETINFO.sn[0], gWIZNETINFO.sn[1], gWIZNETINFO.sn[2], gWIZNETINFO.sn[3]);

    LOG_INFO("task_network.c: Gateway: %d.%d.%d.%d",
             gWIZNETINFO.gw[0], gWIZNETINFO.gw[1], gWIZNETINFO.gw[2], gWIZNETINFO.gw[3]);
}

bool websocket_handshake(uint8_t sock)
{
    LOG_INFO("task_network.c: Performing WebSocket handshake");

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
        LOG_ERROR("task_network.c: Error receiving handshake response");
        return false;
    }

    response[len] = '\0';
    LOG_INFO("task_network.c: Handshake response:\r\n%s", response);

    /* Überprüfen der Antwort */
    if (strstr((char *)response, "HTTP/1.1 101 Switching Protocols") != NULL &&
        strstr((char *)response, "Upgrade: websocket") != NULL &&
        strstr((char *)response, "Connection: Upgrade") != NULL) {
        LOG_INFO("task_network.c: WebSocket handshake successful");
        return true;
    } else {
        LOG_ERROR("task_network.c: WebSocket handshake failed");
        return false;
    }
}

void process_websocket_messages(uint8_t sock)
{
    LOG_INFO("process_websocket_messages: Processing outgoing messages from WebSocket queue");

    MessageForWebSocket msg;
    while (osMessageQueueGet(xWebSocketQueueHandle, &msg, NULL, 0) == osOK) {
        LOG_DEBUG("process_websocket_messages: Retrieved message from queue");
        LOG_DEBUG("process_websocket_messages: message_type=%d, device=%d, target=%d, action=%d, value=%u, json_payload=%s",
                  msg.message_type,
                  msg.device,
                  msg.target,
                  msg.action,
                  msg.value,
                  msg.json_payload);

        send_websocket_message(sock, &msg);
    }

    LOG_INFO("process_websocket_messages: Finished processing WebSocket messages");
}


void send_websocket_message(uint8_t sock, MessageForWebSocket *message)
{
    LOG_INFO("send_websocket_message: Preparing to send WebSocket message");

    const char *json_message;
    char json_message_buffer[256];

    const char *message_type_str = message_type_to_string(message->message_type);
    const char *device_str = device_to_string(message->device);

    LOG_DEBUG("send_websocket_message: Message details - message_type: %d (%s), device: %d (%s), value: %u",
              message->message_type, message_type_str, message->device, device_str, message->value);

    snprintf(json_message_buffer, sizeof(json_message_buffer),
             "{\"UID\":\"%s\",\"message_type\":\"%s\",\"device\":\"%s\",\"value\":%u}",
             uidStr,
             message_type_str,
             device_str,
             message->value);

    json_message = json_message_buffer;

    // Wenn `json_payload` gültige Daten enthält, diese verwenden
    if (message->json_payload[0] != '\0') {
        LOG_INFO("send_websocket_message: Using custom JSON payload");
        json_message = message->json_payload;
    }

    size_t message_len = strlen(json_message);
    LOG_DEBUG("send_websocket_message: JSON message length is %lu", (unsigned long)message_len);

    size_t frame_size;
    uint8_t *websocket_frame;

    // Berechnen der Frame-Größe basierend auf der Nachrichtengröße
    if (message_len <= 125) {
        frame_size = message_len + 6; // 2 Byte Header + 4 Byte Masking Key
    } else if (message_len <= 65535) {
        frame_size = message_len + 8; // 4 Byte Extended Payload + 2 Byte Header + 4 Byte Masking Key
    } else {
        LOG_ERROR("send_websocket_message: Message too long to be sent in a single WebSocket frame");
        return;
    }

    websocket_frame = (uint8_t *)malloc(frame_size);
    if (websocket_frame == NULL) {
        LOG_ERROR("send_websocket_message: Failed to allocate memory for WebSocket frame");
        return;
    }

    websocket_frame[0] = 0x81; // FIN-Bit gesetzt, Text-Frame
    size_t offset;
    if (message_len <= 125) {
        websocket_frame[1] = 0x80 | (uint8_t)message_len; // Mask-Bit gesetzt, Payload-Länge
        offset = 2;
    } else if (message_len <= 65535) {
        websocket_frame[1] = 0x80 | 126; // Mask-Bit gesetzt, Extended Payload-Länge
        websocket_frame[2] = (message_len >> 8) & 0xFF;
        websocket_frame[3] = message_len & 0xFF;
        offset = 4;
    }

    uint8_t masking_key[4];
    for (int i = 0; i < 4; i++) {
        masking_key[i] = rand() % 256;
    }
    memcpy(&websocket_frame[offset], masking_key, 4);
    offset += 4;

    for (size_t i = 0; i < message_len; i++) {
        websocket_frame[offset + i] = json_message[i] ^ masking_key[i % 4];
    }

    LOG_INFO("send_websocket_message: Sending WebSocket frame with length: %lu", (unsigned long)frame_size);

    int32_t total_sent = 0;
    while (total_sent < frame_size) {
        int32_t sent = send(sock, websocket_frame + total_sent, frame_size - total_sent);
        if (sent < 0) {
            LOG_ERROR("send_websocket_message: Failed to send WebSocket frame");
            free(websocket_frame);
            return;
        }
        total_sent += sent;
        LOG_DEBUG("send_websocket_message: Sent %d bytes, total_sent = %d", sent, total_sent);
    }

    LOG_INFO("send_websocket_message: Sent JSON message:\r\n  JSON: %s", json_message);

    free(websocket_frame);
}



void process_received_websocket_data(uint8_t sock, uint8_t *buf, int32_t size)
{
    LOG_INFO("task_network.c: Received data of size %ld bytes", (long int)size);

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

    LOG_DEBUG("task_network.c: Opcode: %d, FIN: %d, Masked: %d, Payload length: %" PRIu64,
              opcode, fin, mask, payload_length);

    /* Unmasking des Payloads */
    if (mask) {
        for (uint64_t i = 0; i < payload_length; i++) {
            buf[header_length + i] ^= masking_key[i % 4];
        }
    }

    /* Sicherstellen, dass der Payload nullterminiert ist */
    buf[header_length + payload_length] = '\0';

    LOG_INFO("task_network.c: Payload: %s", &buf[header_length]);

    /* Verarbeitung des Payloads */
    process_received_data((char *)&buf[header_length]);
}

void process_received_data(const char *json_payload)
{
    LOG_INFO("task_network.c: Processing received JSON data");

    cJSON *root = cJSON_Parse(json_payload);
    if (root == NULL) {
        LOG_ERROR("task_network.c: JSON Parsing Error");
        return;
    }

    cJSON *message_type = cJSON_GetObjectItem(root, "message_type");
    if (message_type == NULL || !cJSON_IsString(message_type)) {
        LOG_ERROR("task_network.c: No message_type found");
        cJSON_Delete(root);
        return;
    }

    LOG_INFO("task_network.c: Message type: %s", message_type->valuestring);

    // Überprüfe die target_UUID
    cJSON *target_UUID = cJSON_GetObjectItem(root, "target_UUID");
    if (target_UUID == NULL || !cJSON_IsString(target_UUID)) {
        LOG_ERROR("task_network.c: No target_UUID found");
        cJSON_Delete(root);
        return;
    }

    // Vergleiche target_UUID mit der Seriennummer des Controllers
    if (strcmp(target_UUID->valuestring, uidStr) != 0) {
        LOG_WARN("task_network.c: target_UUID does not match this controller's UID");
        cJSON_Delete(root);
        return;
    }

    // Aktuelle Zeit synchronisieren, falls vorhanden
    cJSON *current_time = cJSON_GetObjectItem(root, "current_time");
    if (current_time != NULL && cJSON_IsString(current_time)) {
        LOG_INFO("task_network.c: Synchronizing RTC with current_time: %s", current_time->valuestring);
        synchronize_rtc(current_time->valuestring);
    }

    if (strcmp(message_type->valuestring, "newGrowCycle") == 0) {
        parse_new_grow_cycle(root);
    } else if (strcmp(message_type->valuestring, "ControlCommand") == 0) {
        parse_control_command(root);
    } else if (strcmp(message_type->valuestring, "TimeSync") == 0)  {
		handle_timesync_message(root);
    } else if (strcmp(message_type->valuestring, "StatusRequest") == 0) {
        handle_status_request(root);
    } else if (strcmp(message_type->valuestring, "EraseEEPROM") == 0) {
        handle_erase_eeprom_message();
    } else {
        LOG_WARN("task_network.c: Unknown message_type: %s", message_type->valuestring);
    }

    cJSON_Delete(root);
}


void handle_erase_eeprom_message(void) {
    LOG_INFO("task_network.c: Handling EraseEEPROM message");

    uint8_t emptyData[EEPROM_SIZE];
    memset(emptyData, 0xFF, sizeof(emptyData)); // Annahme: 0xFF ist der leere Zustand

    if (EEPROM_Write(0x0000, emptyData, EEPROM_SIZE)) {
        LOG_INFO("task_network.c: EEPROM erased successfully");
    } else {
        LOG_ERROR("task_network.c: Failed to erase EEPROM");
    }
}


void handle_status_request(cJSON *root) {
    LOG_INFO("task_network.c: Handling StatusRequest message");

    cJSON *payload = cJSON_GetObjectItem(root, "payload");
    if (payload == NULL) {
        LOG_ERROR("task_network.c: No payload found in StatusRequest message");
        return;
    }

    cJSON *requested_data = cJSON_GetObjectItem(payload, "requested_data");
    if (requested_data != NULL && cJSON_IsString(requested_data)) {
        if (strcmp(requested_data->valuestring, "GrowCycleConfig") == 0) {
            // Sende GrowCycleConfig zurück
            send_grow_cycle_config();
        } else if (strcmp(requested_data->valuestring, "ControllerState") == 0) {
            // Sende ControllerState zurück
            send_controller_state();
        } else {
            LOG_WARN("task_network.c: Unknown requested_data: %s", requested_data->valuestring);
        }
    } else {
        LOG_ERROR("task_network.c: requested_data not found or invalid in StatusRequest message");
    }
}



void handle_timesync_message(cJSON *root) {
    LOG_INFO("task_network.c: Handling TimeSync message");

    cJSON *payload = cJSON_GetObjectItem(root, "payload");
    if (payload == NULL) {
        LOG_ERROR("task_network.c: No payload found in TimeSync message");
        return;
    }

    cJSON *current_time = cJSON_GetObjectItem(payload, "current_time");
    if (current_time != NULL && cJSON_IsString(current_time)) {
        LOG_INFO("task_network.c: Synchronizing RTC with current_time: %s", current_time->valuestring);
        synchronize_rtc(current_time->valuestring);
    } else {
        LOG_ERROR("task_network.c: current_time not found or invalid in TimeSync message");
    }
}



void send_grow_cycle_config(void) {
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "message_type", "StatusResponse");
    cJSON_AddStringToObject(response, "UID", uidStr);

    cJSON *payload = cJSON_CreateObject();
    cJSON_AddItemToObject(response, "payload", payload);

    osMutexAcquire(gGrowCycleConfigMutexHandle, osWaitForever);
    // Konvertiere gGrowCycleConfig in JSON
    cJSON *config_json = grow_cycle_config_to_json(&gGrowCycleConfig);
    osMutexRelease(gGrowCycleConfigMutexHandle);

    cJSON_AddItemToObject(payload, "GrowCycleConfig", config_json);

    // Sende die Nachricht
    char *json_string = cJSON_PrintUnformatted(response);
    send_json_over_websocket(json_string);
    cJSON_free(json_string);
    cJSON_Delete(response);
}


void send_controller_state(void) {
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "message_type", "StatusResponse");
    cJSON_AddStringToObject(response, "UID", uidStr);

    cJSON *payload = cJSON_CreateObject();
    cJSON_AddItemToObject(response, "payload", payload);

    osMutexAcquire(gControllerStateMutexHandle, osWaitForever);
    // Konvertiere gControllerState in JSON
    cJSON *state_json = controller_state_to_json(&gControllerState);
    osMutexRelease(gControllerStateMutexHandle);

    cJSON_AddItemToObject(payload, "ControllerState", state_json);

    // Sende die Nachricht
    char *json_string = cJSON_PrintUnformatted(response);
    send_json_over_websocket(json_string);
    cJSON_free(json_string);
    cJSON_Delete(response);
}



void parse_control_command(cJSON *root)
{
    LOG_INFO("task_network.c: Parsing ControlCommand");

    cJSON *payload = cJSON_GetObjectItem(root, "payload");
    if (payload == NULL) {
        LOG_ERROR("task_network.c: No payload found in ControlCommand message");
        return;
    }

    cJSON *commands = cJSON_GetObjectItem(payload, "commands");
    if (commands == NULL || !cJSON_IsArray(commands)) {
        LOG_ERROR("task_network.c: No commands array found in payload");
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
                        LOG_ERROR("task_network.c: pump command missing deviceId");
                    }
                } else if (strcmp(target->valuestring, "system") == 0) {
                    handle_system_command(action->valuestring, value);
                } else if (strcmp(target->valuestring, "water") == 0) {
                    handle_water_command(action->valuestring, value);
                } else {
                    LOG_WARN("task_network.c: Unknown target: %s", target->valuestring);
                }
            } else {
                LOG_ERROR("task_network.c: Invalid command format");
            }
        }
    }
}




void parse_new_grow_cycle(cJSON *root)
{
    LOG_INFO("task_network.c: Parsing new grow cycle configuration");

    cJSON *payload = cJSON_GetObjectItem(root, "payload");
    if (payload == NULL) {
        LOG_ERROR("task_network.c: No payload found in newGrowCycle message");
        return;
    }

    cJSON *value = cJSON_GetObjectItem(payload, "value");
    if (value == NULL) {
        LOG_ERROR("task_network.c: No value found in payload");
        return;
    }

    GrowCycleConfig newConfig;
    memset(&newConfig, 0, sizeof(GrowCycleConfig));

    /* startGrowTime */
    cJSON *startGrowTime = cJSON_GetObjectItem(value, "startGrowTime");
    if (startGrowTime != NULL && cJSON_IsString(startGrowTime)) {
        LOG_INFO("task_network.c: startGrowTime: %s", startGrowTime->valuestring);
        strncpy(newConfig.startGrowTime, startGrowTime->valuestring, sizeof(newConfig.startGrowTime));
    } else {
        LOG_ERROR("task_network.c: startGrowTime not found or invalid");
        return;
    }

    /* ledSchedules */
    cJSON *ledSchedules = cJSON_GetObjectItem(value, "ledSchedules");
    if (ledSchedules != NULL && cJSON_IsArray(ledSchedules)) {
        parse_led_schedules(ledSchedules, &newConfig);
    } else {
        LOG_WARN("task_network.c: ledSchedules not found or invalid");
    }

    /* wateringSchedules */
    cJSON *wateringSchedules = cJSON_GetObjectItem(value, "wateringSchedules");
    if (wateringSchedules != NULL && cJSON_IsArray(wateringSchedules)) {
        parse_watering_schedules(wateringSchedules, &newConfig);
    } else {
        LOG_WARN("task_network.c: wateringSchedules not found or invalid");
    }

    // Gültigkeitsprüfung des GrowPlans
    uint32_t totalDuration = calculate_total_duration(&newConfig);
    LOG_INFO("task_network.c: totalDuration = %lu", (unsigned long)totalDuration);

    // Aktuelle Zeit holen
    struct tm currentTime;
    if (!get_current_time(&currentTime)) {
        LOG_ERROR("task_network.c: Failed to get current time from RTC");
        return;
    }

    // Startzeit parsen
    struct tm tm_start;
    const char *start_time_str = newConfig.startGrowTime;
    if (!parse_iso8601_datetime(start_time_str, &tm_start)) {
        LOG_ERROR("task_network.c:\tFailed to parse start time");
        return;
    }


    // Umwandlung in Zeitstempel
    time_t startTimestamp = tm_to_seconds(&tm_start);
    time_t currentTimestamp = tm_to_seconds(&currentTime);

    // Prüfen, ob der GrowPlan abgelaufen ist
    if ((currentTimestamp - startTimestamp) > totalDuration) {
        LOG_WARN("task_network.c: GrowPlan has already expired, not saving configuration");
        return;
    } else {
        LOG_INFO("task_network.c: Grow is still valid");
    }

    /* Speichern der neuen Konfiguration im EEPROM */
    if (save_grow_cycle_config(&newConfig)) {
        LOG_INFO("task_network.c: Grow cycle configuration saved successfully");
    } else {
        LOG_ERROR("task_network.c: Failed to save grow cycle configuration");
    }
}

void parse_led_schedules(cJSON *ledSchedules, GrowCycleConfig *config)
{
    LOG_INFO("task_network.c: Parsing LED schedules");

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
                LOG_INFO("task_network.c: Added LED schedule %d: durationOn=%lu, durationOff=%lu, repetition=%d",
                         i, (unsigned long)config->ledSchedules[i].durationOn,
                         (unsigned long)config->ledSchedules[i].durationOff,
                         config->ledSchedules[i].repetition);
            } else {
                LOG_WARN("task_network.c: Incomplete ledSchedule data");
            }
        }
    }
}

void parse_watering_schedules(cJSON *wateringSchedules, GrowCycleConfig *config)
{
    LOG_INFO("task_network.c: Parsing watering schedules");

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
                LOG_INFO("task_network.c: Added watering schedule %d: duration_full=%lu, duration_empty=%lu, repetition=%d",
                         i, (unsigned long)config->wateringSchedules[i].duration_full,
                         (unsigned long)config->wateringSchedules[i].duration_empty,
                         config->wateringSchedules[i].repetition);
            } else {
                LOG_WARN("task_network.c: Incomplete wateringSchedule data");
            }
        }
    }
}

void synchronize_rtc(const char *iso8601_time)
{
    struct tm tm_time;
    memset(&tm_time, 0, sizeof(struct tm));

    if (!parse_iso8601_datetime(iso8601_time, &tm_time)) {
        LOG_ERROR("task_network.c: Failed to parse current_time");
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
        LOG_INFO("task_network.c: RTC time updated successfully");
    } else {
        LOG_ERROR("task_network.c: Failed to update RTC time");
    }
}

void handle_water_command(const char *action, cJSON *value)
{
    if (strcmp(action, "setState") == 0 && cJSON_IsString(value)) {
        const char *desiredState = value->valuestring;
        LOG_INFO("task_network.c: Forwarding water state command: %s", desiredState);

        // Sende den gewünschten Zustand an die Wassersteuerungs-Task
        WaterCommand waterCmd;
        if (strcmp(desiredState, "full") == 0) {
            waterCmd.commandType = WATER_COMMAND_SET_STATE;
            waterCmd.desiredState = WATER_STATE_FULL;
        } else if (strcmp(desiredState, "empty") == 0) {
            waterCmd.commandType = WATER_COMMAND_SET_STATE;
            waterCmd.desiredState = WATER_STATE_EMPTY;
        } else {
            LOG_WARN("task_network.c: Unknown water state: %s", desiredState);
            return;
        }

        // Sende den Befehl über eine Message Queue
        if (osMessageQueuePut(xWaterCommandQueueHandle, &waterCmd, 0, 0) != osOK) {
            LOG_ERROR("task_network.c: Failed to send water command to water controller");
        }
        LOG_INFO("task_network.c: Finished sending water command to water controller");
    } else {
        LOG_WARN("task_network.c: Unknown action for water: %s", action);
    }
}

void handle_light_command(const char *action, cJSON *value)
{
    if (strcmp(action, "setIntensity") == 0 && cJSON_IsNumber(value)) {
        uint8_t intensity = (uint8_t)value->valueint;
        LOG_INFO("task_network.c: Forwarding light intensity command: %d", intensity);

        // Sende den gewünschten Intensitätswert an die Lichtsteuerungs-Task
        LightCommand lightCmd;
        lightCmd.commandType = LIGHT_COMMAND_SET_INTENSITY;
        lightCmd.intensity = intensity;

        // Sende den Befehl über eine Message Queue
        if (osMessageQueuePut(xLightCommandQueueHandle, &lightCmd, 0, 0) != osOK) {
            LOG_ERROR("task_network.c: Failed to send light command to light controller");
        }
        LOG_INFO("task_network.c: Finished sending light command to light controller");
    } else {
        LOG_WARN("task_network.c: Unknown action for light: %s", action);
    }
}

void handle_pump_command(const char *action, int deviceId, cJSON *value)
{
    if (strcmp(action, "setState") == 0 && cJSON_IsBool(value)) {
        bool enable = cJSON_IsTrue(value);
        LOG_INFO("task_network.c: Setting pump %d state to %s", deviceId, enable ? "ON" : "OFF");

        // Aktualisiere den Controller-Zustand
        UpdatePumpState(enable, deviceId);

        // Sende Befehl an Hardware-Task
        ControlPump(enable, deviceId);
    } else {
        LOG_WARN("task_network.c: Unknown action for pump: %s", action);
    }
}

void handle_system_command(const char *action, cJSON *value)
{
    if (strcmp(action, "setAutomaticMode") == 0 && cJSON_IsBool(value)) {
        bool newAutomaticMode = cJSON_IsTrue(value);
        LOG_INFO("task_network.c: Setting automatic mode to %s", newAutomaticMode ? "ON" : "OFF");

        // Aktualisiere den globalen Wert
        osMutexAcquire(gAutomaticModeHandle, osWaitForever);
        automaticMode = newAutomaticMode;
        osMutexRelease(gAutomaticModeHandle);

        // Speichere nur den automaticMode im EEPROM
        if (!save_automatic_mode(automaticMode)) {
            LOG_ERROR("task_network.c: Failed to save automatic mode");
        }
    } else {
        LOG_WARN("task_network.c: Unknown action for system: %s", action);
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


cJSON *grow_cycle_config_to_json(GrowCycleConfig *config) {
    cJSON *config_json = cJSON_CreateObject();
    if (!config_json) return NULL;

    cJSON_AddStringToObject(config_json, "startGrowTime", config->startGrowTime);

    cJSON *ledSchedules = cJSON_CreateArray();
    for (uint8_t i = 0; i < config->ledScheduleCount; i++) {
        cJSON *schedule = cJSON_CreateObject();
        cJSON_AddNumberToObject(schedule, "durationOn", config->ledSchedules[i].durationOn);
        cJSON_AddNumberToObject(schedule, "durationOff", config->ledSchedules[i].durationOff);
        cJSON_AddNumberToObject(schedule, "repetition", config->ledSchedules[i].repetition);
        cJSON_AddItemToArray(ledSchedules, schedule);
    }
    cJSON_AddItemToObject(config_json, "ledSchedules", ledSchedules);

    cJSON *wateringSchedules = cJSON_CreateArray();
    for (uint8_t i = 0; i < config->wateringScheduleCount; i++) {
        cJSON *schedule = cJSON_CreateObject();
        cJSON_AddNumberToObject(schedule, "duration_full", config->wateringSchedules[i].duration_full);
        cJSON_AddNumberToObject(schedule, "duration_empty", config->wateringSchedules[i].duration_empty);
        cJSON_AddNumberToObject(schedule, "repetition", config->wateringSchedules[i].repetition);
        cJSON_AddItemToArray(wateringSchedules, schedule);
    }
    cJSON_AddItemToObject(config_json, "wateringSchedules", wateringSchedules);

    return config_json;
}


cJSON *controller_state_to_json(ControllerState *state) {
    cJSON *state_json = cJSON_CreateObject();
    if (!state_json) return NULL;

    cJSON_AddBoolToObject(state_json, "wasserbeckenZustand", state->wasserbeckenZustand);
    cJSON_AddBoolToObject(state_json, "pumpeZulauf", state->pumpeZulauf);
    cJSON_AddBoolToObject(state_json, "pumpeAblauf", state->pumpeAblauf);
    cJSON_AddBoolToObject(state_json, "sensorOben", state->sensorOben);
    cJSON_AddBoolToObject(state_json, "sensorUnten", state->sensorUnten);
    cJSON_AddNumberToObject(state_json, "lightIntensity", state->lightIntensity);
    cJSON_AddBoolToObject(state_json, "automaticMode", state->automaticMode);

    return state_json;
}

void send_json_over_websocket(const char *json_string) {
    MessageForWebSocket msg;
    memset(&msg, 0, sizeof(msg));
    msg.message_type = MESSAGE_TYPE_STATUS_RESPONSE;
    strncpy(msg.json_payload, json_string, sizeof(msg.json_payload) - 1);

    // Fügen Sie die Nachricht zur WebSocket-Queue hinzu
    if (osMessageQueuePut(xWebSocketQueueHandle, &msg, 0, 0) != osOK) {
        LOG_ERROR("task_network.c: Failed to send JSON over WebSocket");
    }
}
