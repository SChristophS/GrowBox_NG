#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include "MQTTClient.h"

#define TCP_SOCKET 0

struct opts_struct {
    char* clientid;     // Client ID
    int nodelimiter;    // Flag to indicate whether to use a delimiter
    char* delimiter;    // Delimiter string
    enum QoS qos;       // Quality of Service level
    char* username;     // Username for MQTT broker
    char* password;     // Password for MQTT broker
    char* host;         // Host (IP address) of the MQTT broker
    int port;           // Port of the MQTT broker
    int showtopics;     // Flag to indicate whether to show topics
};

extern struct opts_struct opts;

int mqtt_client_init(Network* n, MQTTClient* c, unsigned char* targetIP, unsigned int targetPort, const char* MQTT_USERNAME, const char* MQTT_PASSWORD);

#endif // MQTT_CLIENT_H