#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>

using namespace websockets;

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
bool shouldConnectWebSocket = false; // Flag, das bestimmt, ob eine WebSocket-Verbindung hergestellt werden soll
bool isWebSocketConnected = false;

String chipId;
WiFiClient espClient;
PubSubClient mqttClient(espClient);
WebsocketsClient websocketsClient;



// Vorwärtsdeklarationen von Funktionen
void connectToBackend();
void sendSensorData();
void processBackendCommand(String message);
void setup_wifi();
void reconnect();
void mqttCallback(char* topic, byte* payload, unsigned int length);

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

void reconnect() {
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
    
    // Hier weitere Verarbeitung deiner MQTT-Nachrichten
    // Wenn es sich um Nachrichten handelt, die für andere Zwecke verarbeitet werden sollten,
    // füge den entsprechenden Code hier ein.
}

void onEventsCallback(WebsocketsEvent event, String data) {
    switch (event) {
        case WebsocketsEvent::ConnectionOpened:
            Serial.println("Event: Connection Opened");
            isWebSocketConnected = true;
            break;
        case WebsocketsEvent::ConnectionClosed:
            Serial.println("Event: Connection Closed");
            isWebSocketConnected = false;
            break;
        case WebsocketsEvent::GotPing:
            Serial.println("Event: Got Ping");
            break;
        case WebsocketsEvent::GotPong:
            Serial.println("Event: Got Pong");
            break;
        default:
            Serial.println("Hier ist etwas schief gegangen");
            break;
    }
}


void onMessageCallback(WebsocketsMessage message) {
  Serial.print("Got Message: ");
  Serial.println(message.data());

  // Parse die JSON-Nachricht
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, message.data());

  // Überprüfe, ob das Parsen erfolgreich war
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  // Extrahiere die Daten
  const char* device = doc["device"]; // "Frontend"
  const char* chipId = doc["chipId"]; // z.B. "1700768"
  const char* msg = doc["message"];   // "CustomAction"
  const char* action = doc["action"]; // "toggleLight"

        Serial.println("device:");
        Serial.println(device);
        Serial.println("chipId:");
        Serial.println(chipId);
        Serial.println("msg:");
        Serial.println(msg);
        Serial.println("action:");
        Serial.println(action);

  if (strcmp(msg, "test_message") == 0) {
        Serial.println("test_message erkannt:");

        // Erstelle und sende die Alive-Nachricht über WebSocket
        StaticJsonDocument<256> doc;
        doc["device"] = "controller";
        doc["chipId"] = chipId;
        doc["message"] = "DingDong deine Ehre is gone";
        String message;
        serializeJson(doc, message);
        websocketsClient.send(message);
        Serial.println("Nachricht über WebSocket gesendet");

  }
}


void connectToBackend() {
    Serial.println("connectToBackend");
    String wsUrl = "ws://" + String(SocketServerHost) + ":" + String(SocketServerPort);

    websocketsClient.onEvent(onEventsCallback);
    websocketsClient.onMessage(onMessageCallback);
    websocketsClient.connect(wsUrl);
    // Du kannst hier prüfen, ob isWebSocketConnected true gesetzt wird,
    // aber beachte, dass connect() asynchron ist.
}

void processBackendCommand(String message) {
    // Implementiere die Logik, die ausgeführt werden soll, wenn eine Nachricht vom Backend empfangen wird.
    Serial.print("Received from backend: ");
    Serial.println(message);
}




void setup() {
    Serial.begin(115200);
    setup_wifi();
    chipId = String(ESP.getChipId());
    mqttClient.setCallback(mqttCallback);
    mqttClient.setServer(mqtt_server, mqtt_port);
    reconnect();
}

void loop() {
    if (!mqttClient.connected()) {
        reconnect();
        Serial.println("reconnect();");
    }
    mqttClient.loop();

    if (isWebSocketConnected) {
        websocketsClient.poll();
    }

    if (shouldConnectWebSocket && !isWebSocketConnected) {
        connectToBackend();
        Serial.println("websocketsClient.poll();");
    }

    unsigned long now = millis();
    
    if (now - lastAliveMessage > aliveInterval) {
        lastAliveMessage = now;

        Serial.println("Publish alive message");
        String fullTopic = "growbox/" + chipId + "/alive";
        mqttClient.publish(fullTopic.c_str(), chipId.c_str());

        if (isWebSocketConnected) {
            // Polling erfolgt hier implizit durch das Verarbeiten von eingehenden Nachrichten oder anderen Aktionen.
            // Falls notwendig, können hier weitere Aktionen hinzugefügt werden, z.B.:
                
            
                // Erstelle und sende die Alive-Nachricht über WebSocket
                StaticJsonDocument<256> doc;
                doc["device"] = "controller";
                doc["chipId"] = chipId;
                doc["message"] = "alive";
                String message;
                serializeJson(doc, message);
                websocketsClient.send(message);
                Serial.println("Alive-Nachricht über WebSocket gesendet");
            }
    }
}
