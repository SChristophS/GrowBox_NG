#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
#include "Cycle.h"
#include "..\include\json.hpp"
#include "Globals.h"

using namespace websockets;
using json = nlohmann::json;

// WLAN-Einstellungen
const char* ssid = "SpatzNetz";
const char* password = "Sandraundchristoph";

// MQTT-Einstellungen
const char* mqtt_server = "192.168.178.25";
const int mqtt_port = 49154;
const char* mqtt_username = "christoph";
const char* mqtt_password = "Aprikose99";

// WebSocket
const char* SocketServerHost = "192.168.178.95";
const int SocketServerPort = 8085;

// Neue Variablen für die zyklische Alive-Nachricht
unsigned long lastAliveMessage = 0; // Letzter Zeitpunkt, zu dem eine Alive-Nachricht gesendet wurde
const long aliveInterval = 10000; // Interval in Millisekunden für Alive-Nachrichten

unsigned long lastLiveDataMessage = 0;
const long LiveDataInterval = 10000;

bool shouldConnectWebSocket = false; // Flag, das bestimmt, ob eine WebSocket-Verbindung hergestellt werden soll
bool isWebSocketConnected = false;
bool liveDataViaSocketActive= false;

int SocketconnectAttempts = 0;
const int maxSocketConnectAttempts = 10; // Maximale Anzahl von Verbindungsversuchen

String chipId;
WiFiClient espClient;
PubSubClient mqttClient(espClient);
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
void connectToSocketBackend();
void sendSensorData();
void setup_wifi();
void mqtt_reconnect();
void mqttCallback(char* topic, byte* payload, unsigned int length);


void debugPrint(DebugLevel level, const String& message) {
    if (level <= currentDebugLevel) {
        Serial.println(message);
    }
}

void setup_wifi() {
    delay(10);
    Serial.println("\nConnecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void mqtt_reconnect() {
    // Achte darauf, dass das korrekte Topic abonniert wird
    String socketConnectTopic = "growbox/" + chipId + "/SocketConnect";

    while (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (mqttClient.connect(chipId.c_str(), mqtt_username, mqtt_password)) {
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

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Received message on topic: ");
    Serial.println(topic);

    String topicStr = String(topic);
    if (topicStr == "growbox/" + chipId + "/SocketConnect") {
        Serial.println("SocketConnect-Nachricht erhalten. Verbinde mit Backend...");
        shouldConnectWebSocket = true; // Setze das Flag, um eine WebSocket-Verbindung herzustellen
        return;
    }
}

void sendRegistrationMessage() {
    StaticJsonDocument<1024> doc;
    doc["device"] = "controller"; // Oder ein anderer geeigneter Wert für Ihr Gerät
    doc["chipId"] = chipId; // Hier verwenden Sie die bereits definierte Variable `chipId`
    doc["message"] = "register"; // Kennzeichnet diese Nachricht als Registrierungsnachricht
    doc["action"] = "register";

    String message;
    serializeJson(doc, message);
    websocketsClient.send(message);
    Serial.println("Registration message sent: " + message);
}

void onEventsCallback(WebsocketsEvent event, String data) {
    switch (event) {
        case WebsocketsEvent::ConnectionOpened:
            Serial.println("Event: Connection Opened");
            isWebSocketConnected = true;
            SocketconnectAttempts = 0; // Zähler zurücksetzen, da die Verbindung erfolgreich war
            sendRegistrationMessage();
            break;
        case WebsocketsEvent::ConnectionClosed:
            Serial.println("Event: Connection Closed");
            isWebSocketConnected = false;
            liveDataViaSocketActive = false;
            break;
        case WebsocketsEvent::GotPing:
            Serial.println("Event: Got Ping");
            break;
        case WebsocketsEvent::GotPong:
            Serial.println("Event: Got Pong");
            break;
        default:
            Serial.println("Something went wrong with WebsocketsEvent");
            break;
    }
}


void onMessageCallback(WebsocketsMessage message) {
    Serial.println("WebSocketMessage: ");
    Serial.println(message.data());

    StaticJsonDocument<4096> doc;
    DeserializationError error = deserializeJson(doc, message.data());

    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
    }
    Serial.println(F("deserializeJson erfolgreich"));

    // Prüfe, ob es sich um eine Nachricht mit "growData" handelt
    if (doc.containsKey("message") && doc["message"].is<JsonObject>()) {
        JsonObject msgObj = doc["message"].as<JsonObject>();
        if (msgObj.containsKey("growData") && msgObj["growData"].is<JsonObject>()) {
            JsonObject growData = msgObj["growData"].as<JsonObject>();

            // Hier können Sie auf die Daten innerhalb von "growData" zugreifen
            int startFromHere = growData["startFromHere"]; // Beispielzugriff auf "startFromHere"
            Serial.print("Start From Here: ");
            Serial.println(startFromHere);

            // Beispiel, wie Sie auf ein Array innerhalb von growData zugreifen könnten
            if (growData.containsKey("ledCycles") && growData["ledCycles"].is<JsonArray>()) {
                JsonArray ledCycles = growData["ledCycles"].as<JsonArray>();
                for (JsonObject ledCycle : ledCycles) {
                    int durationOn = ledCycle["durationOn"];
                    int durationOff = ledCycle["durationOff"];
                    int ledRepetitions = ledCycle["ledRepetitions"];
                    // Führen Sie hier Aktionen mit den LED-Zyklus-Daten aus
                    Serial.print("LED Cycle: On ");
                    Serial.print(durationOn);
                    Serial.print("s, Off ");
                    Serial.print(durationOff);
                    Serial.print("s, Repetitions ");
                    Serial.println(ledRepetitions);
                }
            }
        }
    }
}




void sendLiveDataToSocketBackend(){
        // Erstelle und sende die Alive-Nachricht über WebSocket
        StaticJsonDocument<1024> doc;
        doc["device"] = "controller";
        doc["chipId"] = chipId;
        doc["action"] = "liveData";
        doc["message"] = "DingDong deine Ehre is gone";

        String message;
        serializeJson(doc, message);
        websocketsClient.send(message);
        Serial.println("LiveData Transmitted via WebSocket");
}

void connectToSocketBackend() {
    String wsUrl = "ws://" + String(SocketServerHost) + ":" + String(SocketServerPort);
    //Serial.println("WebSocket-wsUrl:");
    //Serial.print(wsUrl);
    //Serial.println("");

    // register event and message
    websocketsClient.onEvent(onEventsCallback);
    websocketsClient.onMessage(onMessageCallback);
    websocketsClient.connect(wsUrl);
}

void setup() {
    Serial.begin(115200);
    setup_wifi();
    chipId = String(ESP.getChipId());
    mqttClient.setCallback(mqttCallback);
    mqttClient.setServer(mqtt_server, mqtt_port);
    mqtt_reconnect();

    // Stellen Sie sicher, dass das Debugging-Level entsprechend gesetzt ist
    currentDebugLevel = DEBUG_INFO;

    debugPrint(DEBUG_ERROR, "Dies ist ein kritischer Fehler.");
    debugPrint(DEBUG_WARN, "Dies ist eine Warnung.");
    debugPrint(DEBUG_INFO, "Dies ist eine Info-Nachricht.");
    debugPrint(DEBUG_VERBOSE, "Dies ist eine ausführliche Debug-Nachricht.");
}

void loop() {
    if (!mqttClient.connected()) {
        mqtt_reconnect();
        Serial.println("mqtt_reconnect();");
    }
    mqttClient.loop();

    // if websocket is connected, poll
    if (isWebSocketConnected) {
        websocketsClient.poll();
    }

    // if websocket should connect, but isn't yet
    if (shouldConnectWebSocket && !isWebSocketConnected) {
        if (SocketconnectAttempts < maxSocketConnectAttempts) {
            Serial.println("Try to connect to Socket-Backend");
            connectToSocketBackend();
            SocketconnectAttempts++; // Erhöhe den Verbindungsversuchszähler
        } else {
            Serial.println("Max connection attempts reached, stopping further attempts.");
            shouldConnectWebSocket = false; // Stoppe weitere Verbindungsversuche
            SocketconnectAttempts = 0; // Zähler zurücksetzen für den Fall, dass später ein erneuter Verbindungsversuch gestartet wird
        }
    }


    unsigned long now = millis();

    // if LiveData are active, transmit
    if (liveDataViaSocketActive && (now - lastLiveDataMessage > LiveDataInterval)) {
        lastLiveDataMessage = now;
        sendLiveDataToSocketBackend();
    }
    
    if (now - lastAliveMessage > aliveInterval) {
        lastAliveMessage = now;

        // mqtt
        String fullTopic = "growbox/" + chipId + "/alive";
        mqttClient.publish(fullTopic.c_str(), chipId.c_str());
        Serial.println("mqtt-alive-message transmitted");

        // Socket, if connected
        if (isWebSocketConnected) {
                // create the alive-message for socket
                StaticJsonDocument<1024> doc;
                doc["device"] = "controller";
                doc["chipId"] = chipId;
                doc["message"] = "alive";
                doc["now"] = now;
                String message;
                serializeJson(doc, message);

                // and send it
                websocketsClient.send(message);
                Serial.println("socket-alive-message transmitted");
            }
    }
}

