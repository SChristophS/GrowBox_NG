#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
#include "..\include\json.hpp"
#include "Globals.h"
#include "Config.h"
#include <PubSubClient.h>
#include "DebugUtils.h"
#include "mqttHandler.h"
#include "socketHandler.h"
#include "LedController.h"
#include "WiFiSetup.h"
#include "OTAHandler.h"
#include "GrowBoxState.h"
#include "LedObserver.h"

// set current debug level
DebugLevel currentDebugLevel = DEBUG_VERBOSE;

using namespace websockets;
using json = nlohmann::json;

String chipId; // chipID from ESP

unsigned long lastAliveMessage = 0; // Letzter Zeitpunkt, zu dem eine Alive-Nachricht gesendet wurde
int SocketconnectAttempts = 0;
bool GrowIsRunning_old = false;

WiFiClient espClient;
WiFiSetup wifiSetup;
OTAHandler otaHandler;

PubSubClient mqttClient(espClient);
MQTTHandler mqttHandler(mqttClient, MQTT_USERNAME, MQTT_PASSWORD);
SocketHandler socketHandler(SOCKET_SERVER_HOST, SOCKET_SERVER_PORT);
LedController ledController; // Instanziierung des Objekts
WebsocketsClient websocketsClient;


void setup() {
    Serial.begin(115200);

    // setup WIFI
    wifiSetup.setupWiFi(SSID, WIFI_PASSWORD);

    // setup OTA
    otaHandler.setupOTA("ESP8266-OTA");

    globalChipId = String(ESP.getChipId());

    mqttClient.setCallback(MQTTHandler::mqtt_callback);
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttHandler.mqtt_reconnect();

    // Stellen Sie sicher, dass das Debugging-Level entsprechend gesetzt ist
    currentDebugLevel = DEBUG_INFO;

    //debugPrint(DEBUG_ERROR, "Dies ist ein kritischer Fehler.");
    //debugPrint(DEBUG_WARN, "Dies ist eine Warnung.");
    //debugPrint(DEBUG_INFO, "Dies ist eine Info-Nachricht.");
    //debugPrint(DEBUG_VERBOSE, "Dies ist eine ausführliche Debug-Nachricht.");

    pinMode(LEDinternal_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT); 
}

void loop() {
    
    // Objekte erstellen
    //GrowBoxState growBoxState;
    //LedObserver ledObserver(growBoxState);
    
    // and attach the ledObserver
    //growBoxState.attach(&ledObserver);

    while(true){
        // OTA functionality
        ArduinoOTA.handle();

        // mqtt
        if (!mqttClient.connected()) {
            mqttHandler.mqtt_reconnect();
            debugPrint(DEBUG_INFO, "mqtt not connected... try to reconnect");
        }
        mqttClient.loop();  

        // if websocket is connected, poll
        if (isWebSocketConnected) {
            socketHandler.doPoll();
        }         

        // if Grow is active, Update Controller
        if (GrowIsRunning){
            ledController.update();
        }             






        // Implement actual LED change here with analogWrite
        if (LEDstatus) {
                //digitalWrite(LEDinternal_PIN, true); // Bei Systemen mit 10-Bit-PWM. Für 8-Bit-PWM verwenden Sie 255.
                digitalWrite(LED_PIN, 1024);
                debugPrint(DEBUG_INFO, "LED ist eingeschaltet");
        } else {
                //digitalWrite(LEDinternal_PIN, False);
                digitalWrite(LED_PIN, 0);
                debugPrint(DEBUG_INFO, "LED ist ausgeschaltet");
        }


        
        // if websocket is connected and status of GrowIsRunning changes send update
        if ((GrowIsRunning_old != GrowIsRunning) && isWebSocketConnected){
            debugPrint(DEBUG_INFO, ("Detect change in GrowIsRunning -> send update to SocketServer"));
            GrowIsRunning_old = GrowIsRunning;
            socketHandler.sendStatusUpdate();
        }

        // if websocket should connect, but isn't yet
        if (shouldConnectWebSocket && !isWebSocketConnected) {
            if (SocketconnectAttempts < MAX_SOCKET_CONNECT_ATTEMPTS) {
                debugPrint(DEBUG_INFO, ("Try to connect to Socket-Backend"));
                socketHandler.connect();
                socketHandler.sendRegistrationMessage();
                SocketconnectAttempts++;
            } else {
                debugPrint(DEBUG_INFO, ("Max connection attempts reached, stopping further attempts."));
                shouldConnectWebSocket = false;
                SocketconnectAttempts = 0;
            }
        }

        // alive message
        unsigned long now = millis();
        if (now - lastAliveMessage > ALIVE_INTERVAL) {
            lastAliveMessage = now;

            // mqtt
            String fullTopic = "growbox/" + globalChipId + "/alive";
            mqttClient.publish(fullTopic.c_str(), globalChipId.c_str());
            debugPrint(DEBUG_INFO, ("mqtt-alive-message transmitted"));

            // Socket, if connected
            if (isWebSocketConnected) {
                socketHandler.sendAliveMessage();
            }
        }
    }
}