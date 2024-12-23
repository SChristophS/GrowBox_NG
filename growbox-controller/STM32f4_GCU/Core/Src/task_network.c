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


#include "core_cm4.h"

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


#define WEBSOCKET_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"


#define MAX_WEBSOCKET_FRAME_SIZE 65535
uint8_t websocket_frame_buffer[MAX_WEBSOCKET_FRAME_SIZE]; // Statischer Puffer
int websocket_connected = false;

// WebSocket Zustand
typedef enum {
    WS_STATE_HANDSHAKE,
    WS_STATE_CONNECTED
} websocket_state_t;

websocket_state_t websocket_state = WS_STATE_HANDSHAKE;


/* Funktionsprototypen */

/* Hauptfunktion der Netzwerkaufgabe */
void StartNetworkTask(void *argument);
void process_websocket_frame(uint8_t *buf, int32_t size, uint8_t sock);
/* Initialisiert das Netzwerk */
void network_init(void);
void send_pong_frame(uint8_t sock, uint8_t *payload, uint64_t payload_length);
/* Überprüft den Socket-Status und führt entsprechende Aktionen aus */
void check_socket_status(uint8_t *socket_status, uint8_t sock, uint16_t *any_port);

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

/* Sendet registrier message */

void handle_timesync_message(cJSON *root);

void handle_automatic_mode_request(cJSON *root);
void send_manualMode_state(void);
void send_ping_frame(uint8_t sock);



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

	// Registriere den Heartbeat
	if (!RegisterTaskHeartbeat("task_network")) {
		LOG_ERROR("task_network: Failed to register heartbeat");
		Error_Handler();
	}

    // Initialisiere den Zufallszahlengenerator einmalig
    srand((unsigned int)time(NULL));

    /* Netzwerk initialisieren */
    network_init();

    uint8_t sock = 0;
    uint16_t any_port = LOCAL_PORT;
    uint32_t last_ping_time = 0;
    const uint32_t ping_interval = 30000; // 30 Sekunden

    /* Socket initialisieren */
    if (socket(sock, Sn_MR_TCP, any_port++, 0x00) != sock) {
        if (any_port == 0xffff) any_port = 50000;
    }
    LOG_INFO("task_network.c: Socket %d opened", sock);

    for (;;) {
    	UpdateTaskHeartbeat("task_network");

        uint8_t socket_status = getSn_SR(sock);
        //LOG_DEBUG("task_network.c: Socket status after recv: %d", socket_status);

        check_socket_status(&socket_status, sock, &any_port);

        int32_t ret;
        uint16_t size = 0;
        if ((size = getSn_RX_RSR(sock)) > 0) {

            if (size > MAX_BUFFER_SIZE - 1) size = MAX_BUFFER_SIZE - 1;
            memset(DATABUF, 0, MAX_BUFFER_SIZE);
            ret = recv(sock, DATABUF, size);

            if (ret <= 0) {
                int err = getSn_IR(sock);
                LOG_ERROR("task_network.c: Error receiving data. Socket closed. Error code: %d", err);
                close(sock);
                websocket_connected = 0;
            } else {
            	LOG_INFO("task_network.c: process_received_websocket_data");
                process_received_websocket_data(sock, DATABUF, ret);

                uint8_t socket_status_after = getSn_SR(sock);
                LOG_DEBUG("task_network.c: Socket status after processing data: %d", socket_status_after);

                if (socket_status_after == SOCK_CLOSED) {
                    LOG_ERROR("task_network.c: Socket closed after processing data");
                    websocket_connected = 0;
                }
            }
        }

        /* Verarbeite ausgehende Nachrichten */
        if (websocket_connected) {

            uint32_t current_time = osKernelGetTickCount();
             if (current_time - last_ping_time >= ping_interval) {
                 send_ping_frame(sock);
                 last_ping_time = current_time;
             }

            process_websocket_messages(sock);
        }

        osDelay(50);
    }
}

// Funktion zum Verarbeiten empfangener Daten
void handle_received_data(uint8_t sock) {
    uint8_t buffer[MAX_BUFFER_SIZE];
    int32_t len = recv(sock, buffer, sizeof(buffer));
    if (len <= 0) {
        LOG_ERROR("task_network.c: Connection closed or error");
        close(sock);
        websocket_state = WS_STATE_HANDSHAKE;
        return;
    }

    if (websocket_state == WS_STATE_HANDSHAKE) {
        buffer[len] = '\0';
        LOG_INFO("task_network.c: Handshake response:\r\n%s", buffer);

        // Überprüfen der Handshake-Antwort
        if (strstr((char *)buffer, "HTTP/1.1 101 Switching Protocols") &&
            strstr((char *)buffer, "Upgrade: websocket") &&
            strstr((char *)buffer, "Connection: Upgrade")) {
            LOG_INFO("task_network.c: WebSocket handshake status lines verified");

            // Weitere Überprüfungen wie Sec-WebSocket-Accept Verification
            // ...

            websocket_state = WS_STATE_CONNECTED;
            LOG_INFO("task_network.c: WebSocket handshake successful");

            // Sende send_register_message
            // send_register_message();
        } else {
            LOG_ERROR("task_network.c: WebSocket handshake failed");
            close(sock);
        }
    } else if (websocket_state == WS_STATE_CONNECTED) {
        process_websocket_frame(buffer, len, sock);
    }
}

// Funktion zum Starten der Verbindung und des Empfangs
void connect_and_listen(uint8_t sock) {
    while (1) {
        handle_received_data(sock);
        // Nach dem Schließen der Verbindung kann hier ggf. eine Wiederverbindung implementiert werden
    }
}

// Funktion zum Verarbeiten empfangener WebSocket-Frames
void process_websocket_frame(uint8_t *buf, int32_t size, uint8_t sock) {
    if (size < 2) {
        LOG_ERROR("task_network.c: Incomplete WebSocket frame received");
        return;
    }

    uint8_t fin = (buf[0] & 0x80) >> 7;
    uint8_t opcode = buf[0] & 0x0F;
    bool mask = (buf[1] & 0x80) >> 7;
    uint64_t payload_length = buf[1] & 0x7F;
    int offset = 2;

    if (payload_length == 126) {
        if (size < 4) {
            LOG_ERROR("task_network.c: Incomplete extended payload length");
            return;
        }
        payload_length = (buf[2] << 8) | buf[3];
        offset += 2;
    } else if (payload_length == 127) {
        if (size < 10) {
            LOG_ERROR("task_network.c: Incomplete extended payload length");
            return;
        }
        payload_length = 0;
        for (int i = 0; i < 8; i++) {
            payload_length = (payload_length << 8) | buf[offset + i];
        }
        offset += 8;
    }

    uint8_t masking_key[4] = {0};
    if (mask) {
        if (size < offset + 4) {
            LOG_ERROR("task_network.c: Incomplete masking key");
            return;
        }
        memcpy(masking_key, &buf[offset], 4);
        offset += 4;
    }

    if (size < offset + payload_length) {
        LOG_ERROR("task_network.c: Incomplete payload data");
        return;
    }

    uint8_t *payload = &buf[offset];
    if (mask) {
        for (uint64_t i = 0; i < payload_length; i++) {
            payload[i] ^= masking_key[i % 4];
        }
    }

    if (opcode == 0x1) { // Text frame
        // Sicherstellen, dass das Payload null-terminiert ist
        char text_payload[payload_length + 1];
        memcpy(text_payload, payload, payload_length);
        text_payload[payload_length] = '\0';
        LOG_INFO("task_network.c: Received Text Frame: %s", text_payload);
        process_received_data(text_payload);
    } else if (opcode == 0x8) { // Close frame
        LOG_INFO("task_network.c: Received Close Frame");
        close(sock);
        websocket_state = WS_STATE_HANDSHAKE;
    } else if (opcode == 0x9) { // Ping frame
        LOG_INFO("task_network.c: Received Ping Frame, sending Pong");
        send_pong_frame(sock, payload, payload_length);
    } else if (opcode == 0xA) { // Pong frame
        LOG_INFO("task_network.c: Received Pong Frame");
    } else {
        LOG_WARN("task_network.c: Received unsupported WebSocket opcode: %d", opcode);
    }
}

// Funktion zum Senden von Pong-Frames
void send_pong_frame(uint8_t sock, uint8_t *payload, uint64_t payload_length) {
    if (payload_length > MAX_WEBSOCKET_FRAME_SIZE - 6) { // 2 Byte Header + 4 Byte Masking Key
        LOG_ERROR("send_pong_frame: Pong payload too large");
        return;
    }

    uint8_t masking_key[4];
    // Generiere einen zufälligen Masking Key
    for (int i = 0; i < 4; i++) {
        masking_key[i] = rand() % 256;
    }

    // Erstelle den Header
    websocket_frame_buffer[0] = 0x8A; // FIN bit gesetzt, Opcode für Pong ist 0xA
    if (payload_length <= 125) {
        websocket_frame_buffer[1] = 0x80 | payload_length; // Maskenbit gesetzt
        memcpy(&websocket_frame_buffer[2], masking_key, 4); // Masking Key
        // Maskiere den Payload
        for (uint64_t i = 0; i < payload_length; i++) {
            websocket_frame_buffer[6 + i] = payload[i] ^ masking_key[i % 4];
        }
    } else if (payload_length <= 65535) {
        websocket_frame_buffer[1] = 0x80 | 126; // Maskenbit gesetzt, Extended Payload-Länge
        websocket_frame_buffer[2] = (payload_length >> 8) & 0xFF;
        websocket_frame_buffer[3] = payload_length & 0xFF;
        memcpy(&websocket_frame_buffer[4], masking_key, 4); // Masking Key
        // Maskiere den Payload
        for (uint64_t i = 0; i < payload_length; i++) {
            websocket_frame_buffer[8 + i] = payload[i] ^ masking_key[i % 4];
        }
    } else {
        LOG_ERROR("send_pong_frame: Pong payload too large");
        return;
    }

    size_t frame_size = 2 + ((payload_length > 125 && payload_length <= 65535) ? 6 : 6) + payload_length;
    send(sock, websocket_frame_buffer, frame_size);
    LOG_INFO("send_pong_frame: Sent Pong frame with payload length: %" PRIu64, payload_length);
}


void check_socket_status(uint8_t *socket_status, uint8_t sock, uint16_t *any_port)
{
    switch (*socket_status) {
        case SOCK_CLOSED:
            LOG_INFO("task_network.c: Socket %d closed, reopening...", sock);
            if ((socket(sock, Sn_MR_TCP, (*any_port)++, 0x00)) != sock) {
                if (*any_port == 0xffff) *any_port = 50000;
            }
            LOG_INFO("task_network.c: Socket %d opened", sock);
            websocket_connected = 0;
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

            if (!websocket_connected) {
                /* WebSocket-Handshake durchführen */
                if (websocket_handshake(sock)) {
                    LOG_INFO("task_network.c: WebSocket handshake successful");
                    websocket_connected = 1;

                    LOG_INFO("task_network.c: now send_register_message");
                    send_register_message();
                    LOG_INFO("task_network.c: done");


                } else {
                    LOG_ERROR("task_network.c: WebSocket handshake failed");
                    close(sock);
                    websocket_connected = 0;
                }
            }
            break;

        case SOCK_CLOSE_WAIT:
            LOG_INFO("task_network.c: Socket %d close wait", sock);
            disconnect(sock);
            websocket_connected = 0;
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

    // Generieren eines zufälligen 16-Byte-Keys
    uint8_t random_key[16];
    for (int i = 0; i < 16; i++) {
        random_key[i] = rand() % 256;
    }

    // Base64-kodieren des zufälligen Keys
    char sec_websocket_key[25]; // 16 Bytes -> 24 Base64-Zeichen + Nullterminator
    size_t encoded_length = base64_encode(random_key, 16, sec_websocket_key, sizeof(sec_websocket_key) - 1);
    sec_websocket_key[encoded_length] = '\0'; // Nullterminierung

    // Debug: Log des generierten Sec-WebSocket-Key und seiner Länge
    LOG_DEBUG("task_network.c: Generated Sec-WebSocket-Key: %s (Length: %lu)", sec_websocket_key, (unsigned long)encoded_length);

    // Erstellen der Handshake-Anfrage mit korrektem Host und dynamisch generiertem Sec-WebSocket-Key
    char request[512];
    snprintf(request, sizeof(request),
             "GET /chat HTTP/1.1\r\n"
             "Host: 192.168.178.25:8085\r\n"
             "Upgrade: websocket\r\n"
             "Connection: Upgrade\r\n"
             "Sec-WebSocket-Key: %s\r\n"
             "Sec-WebSocket-Version: 13\r\n\r\n",
             sec_websocket_key);

    // Debug: Log der Handshake-Anfrage
    LOG_DEBUG("task_network.c: Sending handshake request:\n%s", request);

    // Senden der Handshake-Anfrage über den Socket
    send(sock, (uint8_t *)request, strlen(request));

    /* Warten auf die Antwort */
    int32_t len;
    uint8_t response[MAX_BUFFER_SIZE];
    osDelay(500); // Wartezeit anpassen, falls nötig

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

        LOG_INFO("task_network.c: WebSocket handshake status lines verified");

        // Extrahieren des Sec-WebSocket-Accept aus der Server-Antwort
        char *accept_ptr = strstr((char *)response, "Sec-WebSocket-Accept:");
        if (accept_ptr) {
            char server_accept[29]; // 28 Base64-Zeichen + Nullterminator
            if (sscanf(accept_ptr, "Sec-WebSocket-Accept: %28s", server_accept) != 1) {
                LOG_ERROR("task_network.c: Failed to parse Sec-WebSocket-Accept");
                return false;
            }

            // Debug: Log der empfangenen Sec-WebSocket-Accept und seiner Länge
            LOG_DEBUG("task_network.c: Received Sec-WebSocket-Accept: %s (Length: %lu)", server_accept, (unsigned long)strlen(server_accept));

            // Berechnen von Sec-WebSocket-Accept
            char concatenated[256];
            snprintf(concatenated, sizeof(concatenated), "%s%s", sec_websocket_key, WEBSOCKET_GUID);

            // Debug: Log des concatenated Strings
            LOG_DEBUG("task_network.c: Concatenated string for SHA-1: %s", concatenated);

            // SHA-1 Hash berechnen
            uint8_t sha1_hash[20];
            sha1((uint8_t *)concatenated, strlen(concatenated), sha1_hash);

            // Debug: Log des SHA-1 Hashes in Hex
            char sha1_hex[41];
            for(int i = 0; i < 20; i++) {
                sprintf(&sha1_hex[i*2], "%02x", sha1_hash[i]);
            }
            sha1_hex[40] = '\0';
            LOG_DEBUG("task_network.c: SHA-1 Hash: %s", sha1_hex);

            // Base64-kodieren des SHA-1 Hashes
            char sec_websocket_accept_calculated[29]; // 20 Bytes -> 28 Base64-Zeichen + Nullterminator
            size_t accept_length = base64_encode(sha1_hash, 20, sec_websocket_accept_calculated, sizeof(sec_websocket_accept_calculated) - 1);
            sec_websocket_accept_calculated[accept_length] = '\0'; // Nullterminierung

            // Debug: Log des berechneten Sec-WebSocket-Accept und seiner Länge
            LOG_DEBUG("task_network.c: Calculated Sec-WebSocket-Accept: %s (Length: %lu)", sec_websocket_accept_calculated, (unsigned long)strlen(sec_websocket_accept_calculated));

            // Vergleichen der berechneten und empfangenen Sec-WebSocket-Accept
            if (strcmp(sec_websocket_accept_calculated, server_accept) == 0) {
                LOG_INFO("task_network.c: Sec-WebSocket-Accept verified");
                return true;
            } else {
                LOG_ERROR("task_network.c: Sec-WebSocket-Accept mismatch");
                LOG_ERROR("task_network.c: Calculated: %s", sec_websocket_accept_calculated);
                LOG_ERROR("task_network.c: Received:   %s", server_accept);
                return false;
            }
        } else {
            LOG_ERROR("task_network.c: Sec-WebSocket-Accept not found in response");
            return false;
        }
    }

    // Fehlender Rückgabewert hinzugefügt
    LOG_ERROR("task_network.c: WebSocket handshake failed - Missing required headers");
    return false;
}

void process_websocket_messages(uint8_t sock) {

    MessageForWebSocket* msg;

    while (osMessageQueueGet(xWebSocketQueueHandle, &msg, NULL, 0) == osOK) {
        LOG_DEBUG("process_websocket_messages: Retrieved message from queue");
        // Nachricht verarbeiten
        send_websocket_message(sock, msg);

        // Nachricht freigeben
        freeMessage(msg);
    }
}

void send_websocket_message(uint8_t sock, MessageForWebSocket *message)
{
    LOG_INFO("send_websocket_message: Preparing to send WebSocket message");

    char json_message[512]; // Passen Sie die Größe entsprechend an
    size_t message_len = 0; // Initialisiere message_len zu 0


    // Wenn `json_payload` gültige Daten enthält, diese verwenden
    if (message->json_payload[0] != '\0') {
        LOG_INFO("send_websocket_message: Using custom JSON payload");
        strncpy(json_message, message->json_payload, sizeof(json_message) - 1);
        json_message[sizeof(json_message) - 1] = '\0'; // Sicherheitshalber terminieren
        message_len = strlen(json_message);
    } else {
        // Bauen Sie die JSON-Nachricht basierend auf message_type, device, etc.
    	const char *message_type_str = message->message_type;
        const char *device_str = device_to_string(message->device);
        const char *changed_value_str = target_to_string(message->target);

        LOG_DEBUG("send_websocket_message: Message details - message_type: %s, device: %d (%s), value: %u",
                  message_type_str, message->device, device_str, message->value);

        snprintf(json_message, sizeof(json_message),
                 "{\"UID\":\"%s\",\"message_type\":\"%s\",\"device\":\"%s\",\"changedValue\":\"%s\",\"value\":%u}",
                 uidStr,
                 message_type_str,
                 device_str,
                 changed_value_str,
                 message->value);

        message_len = strlen(json_message);
    }

    LOG_DEBUG("send_websocket_message: json_message = %s", json_message);
    LOG_DEBUG("send_websocket_message: JSON message length is %lu", (unsigned long)message_len);



    // Vorbereitung des WebSocket-Frames
    size_t frame_size;
    size_t offset = 0;

    if (message_len <= 125) {
        websocket_frame_buffer[offset++] = 0x81; // FIN bit gesetzt, Opcode = 0x1 (Text)
        websocket_frame_buffer[offset++] = 0x80 | (uint8_t)message_len; // Maskenbit gesetzt, Payload-Länge
    } else if (message_len <= 65535) {
        websocket_frame_buffer[offset++] = 0x81; // FIN bit gesetzt, Opcode = 0x1 (Text)
        websocket_frame_buffer[offset++] = 0x80 | 126; // Maskenbit gesetzt, Extended Payload-Länge folgt
        websocket_frame_buffer[offset++] = (message_len >> 8) & 0xFF;
        websocket_frame_buffer[offset++] = message_len & 0xFF;
    } else {
        LOG_ERROR("send_websocket_message: Message too long to be sent in a single WebSocket frame");
        return;
    }


    // Masking Key generieren
    uint8_t masking_key[4];
    for (int i = 0; i < 4; i++) {
        masking_key[i] = rand() % 256;
    }
    memcpy(&websocket_frame_buffer[offset], masking_key, 4);
    offset += 4;

    // Maskiertes Payload
    for (size_t i = 0; i < message_len; i++) {
        websocket_frame_buffer[offset + i] = json_message[i] ^ masking_key[i % 4];
    }
    offset += message_len;

    frame_size = offset;

    LOG_INFO("send_websocket_message: Sending WebSocket frame with length: %lu", (unsigned long)frame_size);

    // Senden des Frames über den Socket
    int32_t total_sent = 0;
    while (total_sent < frame_size) {
        int32_t sent = send(sock, websocket_frame_buffer + total_sent, frame_size - total_sent);
        if (sent < 0) {
            LOG_ERROR("send_websocket_message: Failed to send WebSocket frame");
            return;
        }
        total_sent += sent;
        LOG_DEBUG("send_websocket_message: Sent %d bytes, total_sent = %d", sent, total_sent);
    }

    LOG_INFO("send_websocket_message: Sent JSON message:\r\n  JSON: %s", json_message);
}

void process_received_websocket_data(uint8_t sock, uint8_t *buf, int32_t size)
{
    LOG_INFO("task_network.c: Received data of size %ld bytes", (long int)size);

    /* Parsing the WebSocket frame */
    uint8_t opcode = buf[0] & 0x0F;
    bool fin = (buf[0] & 0x80) != 0;
    bool mask = (buf[1] & 0x80) != 0;
    uint64_t payload_length = buf[1] & 0x7F;
    uint8_t header_length = 2;
    uint8_t masking_key[4] = {0};



    // Handle extended payload lengths (126 and 127)
    if (payload_length == 126) {
        payload_length = (buf[2] << 8) | buf[3];
        header_length += 2;
    } else if (payload_length == 127) {
        payload_length = 0;
        for (int i = 0; i < 8; i++) {
            payload_length = (payload_length << 8) | buf[header_length + i];
        }
        header_length += 8;
    }

    if (header_length + payload_length > size) {
        LOG_ERROR("task_network.c: Payload length exceeds buffer size");
        return;
    }

    if (mask) {
        memcpy(masking_key, &buf[header_length], 4);
        header_length += 4;
    }

    LOG_INFO("task_network.c: Opcode: %d, FIN: %d, Masked: %d, Payload length: %" PRIu64,
             opcode, fin, mask, payload_length);
    LOG_INFO("task_network.c: Opcode: %d, FIN: %d, Masked: %d, Payload length: %llu",
             opcode, fin, mask, (unsigned long long)payload_length);

    // Unmasking the payload if masked
    if (mask) {
        for (uint64_t i = 0; i < payload_length; i++) {
            buf[header_length + i] ^= masking_key[i % 4];
        }
    }

    // Handle different opcodes
    switch (opcode) {
        case 0x1: // Text frame
            // Ensure the payload is null-terminated
            buf[header_length + payload_length] = '\0';
            LOG_INFO("task_network.c: Text Payload: %s", &buf[header_length]);
            process_received_data((char *)&buf[header_length]);
            break;

        case 0x8: // Connection Close frame
            LOG_INFO("task_network.c: Received Connection Close frame");
            close(sock);
            // Set websocket_connected to 0 so that it will reconnect
            websocket_connected = 0;
            break;

        case 0x9: // Ping frame
            LOG_INFO("task_network.c: Received Ping frame, sending Pong");
            // Echo back the payload in a Pong frame
            send_pong_frame(sock, &buf[header_length], payload_length);
            break;

        case 0xA: // Pong frame
            LOG_INFO("task_network.c: Received Pong frame");
            // Typically, no action needed
            break;

        default:
            LOG_WARN("task_network.c: Received unknown opcode: %d", opcode);
            break;
    }
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


		if (strcmp(message_type->valuestring, "newGrowCycle") == 0) {
			parse_new_grow_cycle(root);
		} else if (strcmp(message_type->valuestring, "ControlCommand") == 0) {
			LOG_INFO("task_network.c: Handling ControlCommand message");
			parse_control_command(root);
		} else if (strcmp(message_type->valuestring, "ControllerStateRequest") == 0) {
			LOG_INFO("task_network.c: Handling ControllerStateRequest message");
			send_controller_state();
		} else if (strcmp(message_type->valuestring, "ManualModeRequest") == 0) {
				LOG_INFO("task_network.c: Handling ManualModeRequest message");
				send_manualMode_state();
		} else if (strcmp(message_type->valuestring, "GrowCycleConfigRequest") == 0) {
			LOG_INFO("task_network.c: Handling GrowCycleConfigRequest message");
			send_grow_cycle_config();
		} else if (strcmp(message_type->valuestring, "StatusRequest") == 0) {
			LOG_INFO("task_network.c: Handling StatusRequest message");
			handle_status_request(root);
		} else if (strcmp(message_type->valuestring, "EraseEEPROM") == 0) {
			LOG_INFO("task_network.c: Handling EraseEEPROM message");
			handle_erase_eeprom_message();
		} else if (strcmp(message_type->valuestring, "TimeSync") == 0) {
			LOG_INFO("task_network.c: Handling TimeSync message");
			handle_timesync_message(root);
		} else {
			LOG_WARN("task_network.c: Unknown message_type: %s", message_type->valuestring);
		}

    cJSON_Delete(root);
}

void handle_erase_eeprom_message(void) {
    LOG_INFO("task_network.c: Handling EraseEEPROM message");

    if (EEPROM_Erase()) {
        LOG_INFO("task_network.c: EEPROM erased successfully");
        LOG_INFO("task_network.c: Restarting Controller");
        NVIC_SystemReset();
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
        }
    	else if (strcmp(requested_data->valuestring, "ControllerState") == 0) {
            // Sende ControllerState zurück
            send_controller_state();
        }
    	else if (strcmp(requested_data->valuestring, "ManualMode") == 0) {
            // Sende ControllerState zurück
    		send_manualMode_state();
        }
    	else {
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
    cJSON_AddStringToObject(response, "message_type", MESSAGE_TYPE_GROWCYCLE_CONFIG);
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
    cJSON_AddStringToObject(response, "message_type", MESSAGE_TYPE_CONTROLLER_STATE);
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

void send_manualMode_state(void){
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "message_type", MESSAGE_TYPE_MANUALMODE_STATE);
    cJSON_AddStringToObject(response, "UID", uidStr);

    // Erstelle das Payload-Objekt und füge es zum Response hinzu
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddItemToObject(response, "payload", payload);

    osMutexAcquire(gManualModeMutexHandle, osWaitForever);

    // Füge den Wert von manualMode zum Payload hinzu
    cJSON_AddBoolToObject(payload, "manualMode", manualMode);

    osMutexRelease(gManualModeMutexHandle);

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
   if (strcmp(action, "setManualMode") == 0 && cJSON_IsBool(value)) {
        bool newManualMode = cJSON_IsTrue(value);
        LOG_INFO("task_network.c: Setting manual mode to %s", newManualMode ? "ON" : "OFF");

        // Aktualisiere den globalen Wert
        osMutexAcquire(gManualModeMutexHandle, osWaitForever);
        manualMode = newManualMode;
        osMutexRelease(gManualModeMutexHandle);

        // Speichere nur den automaticMode im EEPROM
        if (!save_manual_mode(manualMode)) {
            LOG_ERROR("task_network.c: Failed to save manual mode");
        }
    } else {
        LOG_WARN("task_network.c: Unknown action for system: %s", action);
    }
}

bool save_manual_mode(bool manualMode) {
osMutexAcquire(gEepromMutexHandle, osWaitForever);
uint16_t address = EEPROM_MANUAL_MODE_ADDR;
printf("task_state_manager.c: Saving manualMode to EEPROM at address 0x%04X\r\n", address);

if (!EEPROM_Write(address, (uint8_t *)&manualMode, sizeof(bool))) {
	printf("task_state_manager.c: Failed to write manualMode to EEPROM\r\n");
	osMutexRelease(gEepromMutexHandle);
	return false;
}

// **Neu hinzugefügt: Lesen des gespeicherten Werts zur Validierung**
bool readBackManualMode;
if (!EEPROM_Read(address, (uint8_t *)&readBackManualMode, sizeof(bool))) {
	printf("task_state_manager.c: Failed to read back manualMode from EEPROM\r\n");
	osMutexRelease(gEepromMutexHandle);
	return false;
}

if (readBackManualMode != manualMode) {
	printf("task_state_manager.c: Read back manualMode does not match saved value\r\n");
	osMutexRelease(gEepromMutexHandle);
	return false;
} else {
	printf("task_state_manager.c: Verified that manualMode was saved correctly\r\n");
}

osMutexRelease(gEepromMutexHandle);

printf("task_state_manager.c: manualMode saved and verified successfully\r\n");
return true;
}

bool load_manual_mode(bool *manualMode) {
    uint16_t address = EEPROM_MANUAL_MODE_ADDR;
    LOG_INFO("state_manager.c: Loading manualMode from EEPROM at address 0x%04X\r\n", address);

    osMutexAcquire(gEepromMutexHandle, osWaitForever);
    if (!EEPROM_Read(address, (uint8_t *)manualMode, sizeof(bool))) {
        printf("task_state_manager.c: Failed to read manualMode from EEPROM\r\n");
        osMutexRelease(gEepromMutexHandle);
        return false;
    }
    osMutexRelease(gEepromMutexHandle);

    LOG_INFO("state_manager.c: manualMode loaded successfully\r\n");
    return true;
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

    if (config_json == NULL) {
        LOG_ERROR("grow_cycle_config_to_json: Failed to create JSON object");
        return NULL;
    }

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

    return state_json;
}

void send_json_over_websocket(const char *json_string) {
    // Nachricht aus dem Nachrichtenpool zuweisen
    MessageForWebSocket* msg = allocateMessage();
    if (msg == NULL) {
        LOG_ERROR("task_network.c: Failed to allocate message for send_json_over_websocket");
        return;
    }

    // Nachricht initialisieren
    msg->message_type = MESSAGE_TYPE_STATUS_RESPONSE;
    strncpy(msg->json_payload, json_string, sizeof(msg->json_payload) - 1);
    msg->json_payload[sizeof(msg->json_payload) - 1] = '\0'; // Sicherheitshalber terminieren

    // Nachricht zur WebSocket-Queue hinzufügen
    if (osMessageQueuePut(xWebSocketQueueHandle, &msg, 0, 0) != osOK) {
        LOG_ERROR("task_network.c: Failed to send JSON over WebSocket");
        freeMessage(msg); // Nachricht freigeben, falls das Hinzufügen fehlschlägt
    } else {
        LOG_INFO("task_network.c: JSON message added to WebSocketQueue");
    }
}

void send_ping_frame(uint8_t sock) {

    uint8_t masking_key[4];
    // Generiere einen zufälligen Masking Key
    for (int i = 0; i < 4; i++) {
        masking_key[i] = rand() % 256;
    }

    uint8_t ping_frame[6]; // 2 Byte Header + 4 Byte Masking Key
    ping_frame[0] = 0x89; // FIN bit gesetzt, Opcode für Ping ist 0x9
    ping_frame[1] = 0x80 | 0x00; // Maskenbit gesetzt, Payload-Länge = 0

    memcpy(&ping_frame[2], masking_key, 4); // Masking Key hinzufügen

    send(sock, ping_frame, 6);
    LOG_INFO("send_ping_frame: Sent Ping frame");
}

