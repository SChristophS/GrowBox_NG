// Config.h
#ifndef CONFIG_H
#define CONFIG_H

// WLAN-Einstellungen
const char* const SSID = "SpatzNetz";
const char* const WIFI_PASSWORD = "Sandraundchristoph";

// MQTT-Einstellungen
const char* const MQTT_SERVER = "192.168.178.25";
const int MQTT_PORT = 49154;
const char* const MQTT_USERNAME = "christoph";
const char* const MQTT_PASSWORD = "Aprikose99";

// WebSocket
const char* const SOCKET_SERVER_HOST = "192.168.178.95";
const int SOCKET_SERVER_PORT = 8085;

// Alive-Nachricht Einstellungen
const long ALIVE_INTERVAL = 10000; // Interval in Millisekunden für Alive-Nachrichten

// Maximale Anzahl von Verbindungsversuchen für WebSocket
const int MAX_SOCKET_CONNECT_ATTEMPTS = 10;

#endif // CONFIG_H
