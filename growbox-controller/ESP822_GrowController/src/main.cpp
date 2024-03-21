#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

// Konfiguration
#define EEPROM_SIZE 2048
#define DATA_START_ADDR 0
#define JSON_BUFFER_SIZE 2048

// WLAN-Einstellungen
const char* ssid = "SpatzNetz";
const char* password = "Sandraundchristoph";

// MQTT-Einstellungen
const char* mqtt_server = "192.168.178.25";
const int mqtt_port = 49154;
const char* mqtt_username = "christoph";
const char* mqtt_password = "Aprikose99";

const long pingInterval = 10000; // Ping-Intervall in Millisekunden
long lastPing = 0;

String chipId;

WiFiClient espClient;
PubSubClient client(espClient);

// Neuer TCP-Client für Backend-Verbindung
WiFiClient backendClient;

const char* backendHost = "192.168.178.95"; // Adresse deines Backends
const int backendPort = 8085; // Neuer Port für Backend-Verbindung

// Vorwärtsdeklarationen
void connectToBackend();
void sendSensorData();
void processBackendCommand(String message);

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
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (client.connect(chipId.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("Connected");
            client.subscribe(socketConnectTopic.c_str());
            Serial.print("Subscribed to: ");
            Serial.println(socketConnectTopic);
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client.state());
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
        if (!backendClient.connected()) {
            connectToBackend();
        }
        return;
    }
    
    // Hier weitere Verarbeitung deiner MQTT-Nachrichten
    // Wenn es sich um Nachrichten handelt, die für andere Zwecke verarbeitet werden sollten,
    // füge den entsprechenden Code hier ein.
}

void connectToBackend() {
    if (!backendClient.connect(backendHost, backendPort)) {
        Serial.println("Verbindung zum Backend fehlgeschlagen!");
    } else {
        Serial.println("Verbunden mit Backend!");
        // Senden einer Begrüßungsnachricht oder ähnliches nach erfolgreicher Verbindung
        backendClient.println("Hello from ESP8266");
    }
}

void setup() {
    Serial.begin(115200);
    setup_wifi();
    chipId = String(ESP.getChipId());
    client.setCallback(mqttCallback);
    client.setServer(mqtt_server, mqtt_port);
    reconnect();
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    // Überprüfung auf Daten vom Backend, wenn bereits verbunden
    if (backendClient.connected()) {
        if (backendClient.available()) {
            String line = backendClient.readStringUntil('\n');
            processBackendCommand(line);
        }
    } else {
        // Hier könnten wir versuchen, die Verbindung erneut herzustellen, falls gewünscht.
        // Zum Beispiel durch erneutes Senden der MQTT-Nachricht oder ähnliches.
    }

    long now = millis();
    if (now - lastPing > pingInterval) {
        lastPing = now;
        Serial.println("Publish alive message");
        String fullTopic = "growbox/" + chipId + "/alive/";
        client.publish(fullTopic.c_str(), "your_status_message");
    }
}

void processBackendCommand(String message) {
    // Implementiere die Logik, die ausgeführt werden soll, wenn eine Nachricht vom Backend empfangen wird.
    Serial.print("Received from backend: ");
    Serial.println(message);
}
