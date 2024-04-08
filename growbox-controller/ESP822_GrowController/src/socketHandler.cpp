#include "socketHandler.h"
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
#include "Globals.h"
#include "LedController.h"


extern LedController ledController; // Externe Deklaration
extern WebsocketsClient websocketsClient;

SocketHandler::SocketHandler(const char* serverHost, int serverPort)
    : serverHost(serverHost), serverPort(serverPort) {

    websocketsClient.onMessage(SocketHandler::onMessageCallback);
    websocketsClient.onEvent(SocketHandler::onEventsCallback);

}

void SocketHandler::doPoll(){
    websocketsClient.poll();
}

void SocketHandler::connect() {
    String wsUrl = "ws://" + String(serverHost) + ":" + String(serverPort);
    websocketsClient.connect(wsUrl);
}

void SocketHandler::sendRegistrationMessage() {
    StaticJsonDocument<1024> doc;
    doc["device"] = "controller";
    doc["chipId"] = globalChipId;
    doc["message"] = "register";
    doc["action"] = "register";

    String message;
    serializeJson(doc, message);
    websocketsClient.send(message);
    Serial.println("Registration message sent: " + message);
}

void SocketHandler::sendAliveMessage() {
    // create the alive-message for socket
    StaticJsonDocument<1024> doc;
    doc["device"] = "controller";
    doc["chipId"] = globalChipId;
    doc["message"] = "alive";

    String message;
    serializeJson(doc, message);

    // and send it
    websocketsClient.send(message);
    Serial.println("socket-alive-message transmitted");
}

void SocketHandler::sendStatusUpdate() {
    StaticJsonDocument<1024> doc;
    doc["device"] = "controller";
    doc["chipId"] = globalChipId;
    doc["action"] = "StatusUpdate";
    doc["running"] = GrowIsRunning;

    String message;
    serializeJson(doc, message);

    // and send it
    websocketsClient.send(message);
    Serial.println("socket-status-message transmitted");
}


void SocketHandler::onMessageCallback(WebsocketsMessage message) {
    Serial.println("New Message received");
    Serial.println(message.data());

    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, message.data());

    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    } else {
        Serial.print(F("deserializeJson() was ok: "));
    }

    const char* action = doc["action"]; // Versucht den Wert von action zu holen

    if (action == nullptr) {
        Serial.println(F("Der Schlüssel 'action' existiert nicht."));
        return;
    } else if (strcmp(action, "new_growplan") == 0) {
        Serial.println(F("'action' ist 'new_growplan'."));

        // Zugriff auf das verschachtelte "message" Objekt
        JsonObject msg = doc["message"];
        if (!msg.isNull()) {
            // Konvertiere das "message" Objekt (oder einen Teil davon) zurück in einen String
            // wenn nötig, um es an die LedController::parseJson Funktion zu übergeben.
            String msgStr;
            serializeJson(msg, msgStr); // Hier nutzen wir das "message" Objekt direkt
            
            // Übergebe die "message" (die growData enthält) an die parseJson Methode von LedController
            extern LedController ledController; // Stellen Sie sicher, dass dies in der Datei deklariert ist
            ledController.parseJson(msgStr.c_str());
        } else {
            Serial.println(F("Kein 'message' Objekt gefunden."));
        }
    } else if (strcmp(action, "startGrow") == 0) {
        GrowIsRunning = true;
        Serial.println("Set GrowIsRunning to true");
    } else if (strcmp(action, "stopGrow") == 0) {
        GrowIsRunning = false;
        Serial.println("Set GrowIsRunning to false");
    } else {
        Serial.println(F("'action' hat einen anderen Wert."));
    }
}



void SocketHandler::onEventsCallback(WebsocketsEvent event, String data) {
    // Behandle WebSocket-Ereignisse hier
    if (event == WebsocketsEvent::ConnectionOpened) {
        Serial.println("WebSocket Connection Opened");
        isWebSocketConnected = true;

    } else if (event == WebsocketsEvent::ConnectionClosed) {
        Serial.println("WebSocket Connection Closed");
        isWebSocketConnected = false;

    } else if (event == WebsocketsEvent::GotPing) {
            Serial.println("Event: Got Ping");

    } else if (event == WebsocketsEvent::GotPong){
            Serial.println("Event: Got Pong");

    } else {
        Serial.println("Something went wrong with WebsocketsEvent");
    };

}


