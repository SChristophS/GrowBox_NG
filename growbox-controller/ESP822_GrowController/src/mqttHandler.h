// mqttHandler.h
#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include "Globals.h"


class MQTTHandler {
public:
    // Aktualisiere den Konstruktor, um die chipId zu entfernen
    MQTTHandler(PubSubClient& client, const char* username, const char* password);
    void mqtt_reconnect();
    static void mqtt_callback(char* topic, byte* payload, unsigned int length);


private:
    PubSubClient& mqttClient;
    const char* mqttUsername;
    const char* mqttPassword;
};

#endif // MQTT_HANDLER_H
