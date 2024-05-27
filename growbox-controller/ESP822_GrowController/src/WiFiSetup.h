#ifndef WiFiSetup_h
#define WiFiSetup_h

#include <ESP8266WiFi.h>

class WiFiSetup {
public:
    void setupWiFi(const char* ssid, const char* password);
};

#endif
