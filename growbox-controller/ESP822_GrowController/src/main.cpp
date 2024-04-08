#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
//#include "Cycle.h"
#include "..\include\json.hpp"
#include "Globals.h"
#include "Config.h"
#include <PubSubClient.h>

#include "mqttHandler.h"
#include "socketHandler.h"
#include "LedController.h"

using namespace websockets;
using json = nlohmann::json;

String chipId; // chipID from ESP

// Neue Variablen für die zyklische Alive-Nachricht
unsigned long lastAliveMessage = 0; // Letzter Zeitpunkt, zu dem eine Alive-Nachricht gesendet wurde

unsigned long lastLiveDataMessage = 0;

bool liveDataViaSocketActive= false;
int SocketconnectAttempts = 0;
bool GrowIsRunning_old = false;

extern bool GrowIsRunning;

WiFiClient espClient;
PubSubClient mqttClient(espClient);
MQTTHandler mqttHandler(mqttClient, MQTT_USERNAME, MQTT_PASSWORD);
SocketHandler socketHandler(SOCKET_SERVER_HOST, SOCKET_SERVER_PORT);
LedController ledController; // Instanziierung des Objekts
WebsocketsClient websocketsClient;

enum DebugLevel {
    DEBUG_NONE = 0, // Keine Debug-Nachrichten
    DEBUG_ERROR,    // Nur Fehlermeldungen
    DEBUG_WARN,     // Warnungen und Fehler
    DEBUG_INFO,     // Informative Nachrichten, Warnungen und Fehler
    DEBUG_VERBOSE   // Alle Nachrichten
};

// set current debug level
DebugLevel currentDebugLevel = DEBUG_VERBOSE;




// Vorwärtsdeklarationen von Funktionen
void setup_wifi();



void debugPrint(DebugLevel level, const String& message) {
    if (level <= currentDebugLevel) {
        Serial.println(message);
    }
}

void setup_wifi() {
    delay(10);
    Serial.println("\nConnecting to ");
    Serial.println(SSID);

    WiFi.begin(SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void setup() {
    Serial.begin(115200);
    setup_wifi();
    globalChipId = String(ESP.getChipId());

    mqttClient.setCallback(MQTTHandler::mqtt_callback);
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttHandler.mqtt_reconnect();

    // Stellen Sie sicher, dass das Debugging-Level entsprechend gesetzt ist
    currentDebugLevel = DEBUG_INFO;

    debugPrint(DEBUG_ERROR, "Dies ist ein kritischer Fehler.");
    debugPrint(DEBUG_WARN, "Dies ist eine Warnung.");
    debugPrint(DEBUG_INFO, "Dies ist eine Info-Nachricht.");
    debugPrint(DEBUG_VERBOSE, "Dies ist eine ausführliche Debug-Nachricht.");

    pinMode(D0, OUTPUT); 
}

void loop() {
    if (!mqttClient.connected()) {
        mqttHandler.mqtt_reconnect();
        Serial.println("mqttHandler.mqtt_reconnect();");
    }
    mqttClient.loop();

    // if websocket is connected, poll
    if (isWebSocketConnected) {
        socketHandler.doPoll();
    }

    if (GrowIsRunning){
        ledController.update();
    }

     

    if (GrowIsRunning_old != GrowIsRunning){
        Serial.println("Detect change in GrowIsRunning -> send update to SocketServer");
        GrowIsRunning_old = GrowIsRunning;
        socketHandler.sendStatusUpdate();
    }

    // if websocket should connect, but isn't yet
    if (shouldConnectWebSocket && !isWebSocketConnected) {
        if (SocketconnectAttempts < MAX_SOCKET_CONNECT_ATTEMPTS) {
            Serial.println("Try to connect to Socket-Backend");
            socketHandler.connect();
            socketHandler.sendRegistrationMessage();
            SocketconnectAttempts++; // Erhöhe den Verbindungsversuchszähler
        } else {
            Serial.println("Max connection attempts reached, stopping further attempts.");
            shouldConnectWebSocket = false; // Stoppe weitere Verbindungsversuche
            SocketconnectAttempts = 0; // Zähler zurücksetzen für den Fall, dass später ein erneuter Verbindungsversuch gestartet wird
        }
    }


    unsigned long now = millis();

      if (now - lastAliveMessage > ALIVE_INTERVAL) {
        lastAliveMessage = now;

        // mqtt
        String fullTopic = "growbox/" + globalChipId + "/alive";

        // Konvertiere globalChipId in ein const char* und übergebe die Nachricht
        //mqttClient.publish(fullTopic.c_str(), globalChipId.c_str(), strlen(globalChipId.c_str()));
        mqttClient.publish(fullTopic.c_str(), globalChipId.c_str());

        Serial.println("mqtt-alive-message transmitted");

        // Socket, if connected
        if (isWebSocketConnected) {
            socketHandler.sendAliveMessage();
        }
    }
}

