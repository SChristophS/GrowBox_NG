#ifndef SOCKET_HANDLER_H
#define SOCKET_HANDLER_H

#include <Arduino.h>
#include <ArduinoWebsockets.h>
//#include <ArduinoJson.h>
#include "Globals.h"


using namespace websockets;

class SocketHandler {
public:
    SocketHandler(const char* serverHost, int serverPort);
    void connect();
    void sendRegistrationMessage();
    void sendAliveMessage();
    void doPoll();
    void sendStatusUpdate();

private:
    const char* serverHost;
    int serverPort;
    WebsocketsClient websocketsClient;

    static void onEventsCallback(WebsocketsEvent event, String data);
    static void onMessageCallback(WebsocketsMessage message);
};

#endif // SOCKET_HANDLER_H
