#ifndef OTAHandler_h
#define OTAHandler_h

#include <ArduinoOTA.h>

class OTAHandler {
public:
    void setupOTA(const char* hostname, const char* password = NULL);
};

#endif
