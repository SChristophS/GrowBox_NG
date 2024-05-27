#include "MQTTHandler.h"
#include "Globals.h"
#include "Config.h"
#include <PubSubClient.h>


//bool MQTTHandler::shouldConnectWebSocket = false;

MQTTHandler::MQTTHandler(PubSubClient& client, const char* username, const char* password)
: mqttClient(client), mqttUsername(username), mqttPassword(password) {}

void MQTTHandler::mqtt_reconnect() {
    // Verwende globalChipId anstelle von chipId
    String socketConnectTopic = "growbox/" + globalChipId + "/SocketConnect";

    while (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Verwende .c_str() auf globalChipId für den connect Aufruf
        if (mqttClient.connect(globalChipId.c_str(), mqttUsername, mqttPassword)) {
            Serial.println("Connected");
            mqttClient.subscribe(socketConnectTopic.c_str());
            Serial.print("Subscribed to: ");
            Serial.println(socketConnectTopic);
        } else {
            Serial.print("Failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void MQTTHandler::mqtt_callback(char* topic, byte* payload, unsigned int length) {
    // Verbesserte Klarheit durch Aufteilung der Operationen
    String expectedTopic = "growbox/" + globalChipId + "/SocketConnect";
    if (String(topic) == expectedTopic) {
        Serial.println("SocketConnect-Nachricht erhalten. Verbinde mit Backend...");
        shouldConnectWebSocket = true;
    }
}

