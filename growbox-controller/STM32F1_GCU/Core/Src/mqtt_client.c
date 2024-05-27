#include "mqtt_client.h"
#include "MQTTClient.h"
#include <stdio.h>
#include <string.h> // Für memcpy
#include "uart_redirect.h"
#include "stm32f1xx_hal.h" //für uid

// Puffergröße Definition
#define BUFFER_SIZE 2048

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

    if (opts.showtopics)
    {
        memcpy(testbuffer, (char*)message->payload, (int)message->payloadlen);
        *(testbuffer + (int)message->payloadlen) = '\n'; // Zeichen als Char zuweisen
        printf("%s\r\n", testbuffer);
    }

    if (opts.nodelimiter)
        printf("%.*s", (int)message->payloadlen, (char*)message->payload);
    else
        printf("%.*s%s", (int)message->payloadlen, (char*)message->payload, opts.delimiter);
}

int mqtt_client_init(Network* n, MQTTClient* c, unsigned char* targetIP, unsigned int targetPort, const char* MQTT_USERNAME, const char* MQTT_PASSWORD)
{
    char uidStr[25];
    GetSTM32UID(uidStr);

    printf("STM32 UID: %s\n", uidStr);

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
    data.clientID.cstring = uidStr;
    data.username.cstring = (char*)MQTT_USERNAME;
    data.password.cstring = (char*)MQTT_PASSWORD;

    data.keepAliveInterval = 60;
    data.cleansession = 1;

    rc = MQTTConnect(c, &data);
    printf("Connected %d\r\n", rc);
    opts.showtopics = 1;

    printf("Subscribing to %s\r\n", "hello/wiznet");
    rc = MQTTSubscribe(c, "hello/wiznet", opts.qos, messageArrived);
    printf("Subscribed %d\r\n", rc);

    return rc;
}

