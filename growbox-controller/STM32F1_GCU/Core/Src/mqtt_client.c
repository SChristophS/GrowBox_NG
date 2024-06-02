#include "mqtt_client.h"
#include "MQTTClient.h"
#include <stdio.h>
#include <string.h> // Für memcpy
#include "uart_redirect.h"
#include "stm32f1xx_hal.h" //für uid
#include <string.h>
#include <stdio.h>
#include <cJSON.h>
#include <stdbool.h>

// Puffergröße Definition
#define BUFFER_SIZE 2048

extern bool connect_to_backend;

// Puffer zum Empfangen von Daten
unsigned char tempBuffer[BUFFER_SIZE] = {};

// MQTT Client Optionen
struct opts_struct opts = {
    .clientid = (char*)"stdout-subscriber",
    .nodelimiter = 0,
    .delimiter = (char*)"\n",
    .qos = QOS0,
    .username = NULL,
    .password = NULL,
    .host = NULL,
    .port = 0,
    .showtopics = 0
};


void GetSTM32UID(char *uidStr) {
    uint32_t uid[3];
    uid[0] = *(uint32_t *)0x1FFFF7E8;
    uid[1] = *(uint32_t *)0x1FFFF7EC;
    uid[2] = *(uint32_t *)0x1FFFF7F0;

    sprintf(uidStr, "%08lX%08lX%08lX", uid[0], uid[1], uid[2]);
}

// Callback-Funktion für eingehende Nachrichten
void messageArrived(MessageData* md)
{
    unsigned char testbuffer[100];
    MQTTMessage* message = md->message;

    // Nachricht in den Puffer kopieren und nullterminieren
    memcpy(testbuffer, (char*)message->payload, (int)message->payloadlen);
    testbuffer[(int)message->payloadlen] = '\0';

    // Nachricht drucken, falls showtopics aktiviert ist
    if (opts.showtopics)
    {
        printf("Message received: %s\r\n", testbuffer);
    }

    // Nachricht als JSON parsen
    cJSON *json = cJSON_Parse((char*)testbuffer);
    if (json == NULL)
    {
        printf("Error parsing JSON\n");
        return;
    }

    // JSON-Wert von "action" extrahieren
    cJSON *action = cJSON_GetObjectItemCaseSensitive(json, "action");
    if (cJSON_IsString(action) && (action->valuestring != NULL))
    {
        printf("Action: %s\r\n", action->valuestring);

        // switch-Anweisung für verschiedene Aktionen
        if (strcmp(action->valuestring, "connect_socket") == 0)
        {
            printf("Connecting to socketServer...\r\n");
            connect_to_backend = true;
            // Hier den Code für das Verbinden des Sockets einfügen
        }
        else
        {
            printf("Unknown action: %s\n", action->valuestring);
        }
    }
    else
    {
        printf("No action found in message\n");
    }

    cJSON_Delete(json);

    // Zusätzliche Nachrichtenverarbeitung
    if (opts.nodelimiter)
        printf("%.*s", (int)message->payloadlen, (char*)message->payload);
    else
        printf("%.*s%s", (int)message->payloadlen, (char*)message->payload, opts.delimiter);
}

int mqtt_client_init(Network* n, MQTTClient* c, unsigned char* targetIP, unsigned int targetPort, const char* MQTT_USERNAME, const char* MQTT_PASSWORD)
{


    unsigned char buf[100];
    int rc;

    opts.host = (char*)targetIP;
    opts.port = targetPort;

    NewNetwork(n, TCP_SOCKET);
    ConnectNetwork(n, targetIP, targetPort); // Korrigierter Aufruf von ConnectNetwork
    MQTTClientInit(c, n, 1000, buf, 100, tempBuffer, BUFFER_SIZE);

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.willFlag = 0;
    data.MQTTVersion = 3;
    data.clientID.cstring = "TEST_STM32";
    data.username.cstring = (char*)MQTT_USERNAME;
    data.password.cstring = (char*)MQTT_PASSWORD;

    data.keepAliveInterval = 60;
    data.cleansession = 1;

    rc = MQTTConnect(c, &data);
    printf("Connected %d\r\n", rc);
    opts.showtopics = 1;

    char topic[50];
    static char global_topic[50];
    static char global_uidStr[25];

    char uidStr[25];
    GetSTM32UID(uidStr);

    printf("STM32 UID: %s\n", uidStr);

    GetSTM32UID(global_uidStr);
    printf("global_uidStr STM32 UID: %s\n", global_uidStr);

    sprintf(global_topic, "growbox/%s/SocketConnect", global_uidStr);
    printf("Subscribing to %s\r\n", global_topic);

    // Abonnieren des Themas
    rc = MQTTSubscribe(c, global_topic, opts.qos, messageArrived);
    printf("Subscribed to %s with result %d\r\n", global_topic, rc);

    // Zusätzliche Fehlerprüfung
    if (rc != 0) {
        printf("Fehler beim Abonnieren des Themas %s, Rückgabewert: %d\r\n", global_topic, rc);
    }




    return rc;
}


