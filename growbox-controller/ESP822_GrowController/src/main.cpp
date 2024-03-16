#include <ESP8266WiFi.h>

#define MQTT_MAX_PACKET_SIZE 2048
#include <PubSubClient.h>

#include <EEPROM.h>
#include <ArduinoJson.h>

#define EEPROM_SIZE 2048
#define DATA_START_ADDR 0

#define JSON_BUFFER_SIZE 2048 // Anpassen, falls nötig
StaticJsonDocument<JSON_BUFFER_SIZE> doc;


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
            Serial.println("Connected");
            // Verschiebe das Abonnieren hierher
            String subscribeTopic = "growbox/" + chipId + "/newGrowplan";
            // client.subscribe(subscribeTopic.c_str());
            client.subscribe("growbox/1704975/newGrowplan");

            Serial.print("Subscribed to: ");
            Serial.println(subscribeTopic);
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}


// Funktion zum Speichern der Growplan-ID
void saveGrowplanId(String id) {
  EEPROM.begin(EEPROM_SIZE);
for (unsigned int i = 0; i < id.length(); ++i) {
    EEPROM.write(DATA_START_ADDR + i, id[i]);
  }
  EEPROM.commit();
  EEPROM.end();
}

// Funktion zum Laden der Growplan-ID
String loadGrowplanId() {
  EEPROM.begin(EEPROM_SIZE);
  String id = "";
  for (int i = DATA_START_ADDR; i < DATA_START_ADDR + 32; ++i) { // Annahme: ID ist max. 32 Zeichen lang
    char c = EEPROM.read(i);
    if (c == 0) break;
    id += c;
  }
  EEPROM.end();
  return id;
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Received message on topic: ");
  Serial.println(topic);

  String receivedPayload;
  for (unsigned int i = 0; i < length; i++) {
    receivedPayload += (char)payload[i];
  }
  Serial.print("Payload: ");
  Serial.println(receivedPayload);

  // Additional detailed logging
  Serial.println("Trying to parse JSON...");
  StaticJsonDocument<JSON_BUFFER_SIZE> doc;
  DeserializationError error = deserializeJson(doc, receivedPayload);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  // Check if "planId" field exists
  if (!doc.containsKey("planId")) {
    Serial.println("Error: Missing 'planId' field in message!");
    return;
  }

  String planId = doc["planId"];
  Serial.print("Plan ID: ");
  Serial.println(planId);

  String currentPlanId = loadGrowplanId();
  Serial.print("Current Plan ID: ");
  Serial.println(currentPlanId);

  if (planId != currentPlanId) {
    Serial.println("New plan detected. Processing...");
    saveGrowplanId(planId);
  } else {
    Serial.println("Received plan is the same as the current one. Ignoring...");
  }
}


void setup() {
    Serial.begin(115200);
    setup_wifi();
    chipId = String(ESP.getChipId());
    Serial.print("chipId: ");
    Serial.println(chipId);

    client.setCallback(mqttCallback);
    // Stellen Sie sicher, dass der ESP8266 das korrekte Topic abonniert
    String subscribeTopic = "growbox/" + chipId + "/newGrowplan";
    client.subscribe(subscribeTopic.c_str());
    Serial.print("subscribeTopic: ");
    Serial.println(subscribeTopic.c_str());

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
        Serial.println("publish alive message");

        // Construct the full topic using chipId
        String fullTopic = "growbox/" + chipId + "/alive/";
        client.publish(fullTopic.c_str(), "your_status_message");  // Replace with your actual status message
    }
}
