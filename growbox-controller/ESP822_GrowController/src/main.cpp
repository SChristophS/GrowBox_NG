#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// WLAN settings
const char* ssid = "SpatzNetz";
const char* password = "Sandraundchristoph";

// MQTT Settings
const char* mqtt_server = "192.168.178.25";
const int mqtt_port = 49154;
const char* mqtt_username = "christoph";
const char* mqtt_password = "Aprikose99";

const long pingInterval = 10000; // Ping interval in milliseconds
long lastPing = 0;

String chipId;

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");

        if (client.connect(chipId.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("connected");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void setup() {
    Serial.begin(115200);
    setup_wifi();
    chipId = String(ESP.getChipId());

    client.setServer(mqtt_server, mqtt_port);
    reconnect();
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    long now = millis();
    if (now - lastPing > pingInterval) {
        // Time to send another ping
        lastPing = now;
        Serial.println("publish status message");
        client.publish("growbox/status", chipId.c_str());
    }
}
