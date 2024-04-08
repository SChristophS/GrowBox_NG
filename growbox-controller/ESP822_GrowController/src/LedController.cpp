#include "LedController.h"

LedController::LedController() {}

void LedController::parseJson(const char* json) {
    DynamicJsonDocument doc(10240); // Adjust size as needed
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
    }

    JsonArray ledCycles = doc["growData"]["ledCycles"].as<JsonArray>();
    for (JsonObject obj : ledCycles) {
        LedCycle cycle = {
            obj["durationOn"],
            obj["durationOff"],
            obj["ledRepetitions"]
        };
        cycles.push_back(cycle);
        debugPrintCycle(cycle);
    }
}

void LedController::update() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 1000) { // Update every second
        previousMillis = currentMillis;
        if (cycleIndex < cycles.size()) {
            LedCycle& cycle = cycles[cycleIndex];
            if (currentStateTime < cycle.durationOn) {
                changeLedStatus(true);
            } else if (currentStateTime < cycle.durationOn + cycle.durationOff) {
                changeLedStatus(false);
            }
            currentStateTime++;
            if (currentStateTime >= (cycle.durationOn + cycle.durationOff)) {
                currentStateTime = 0;
                currentRepetition++;
                if (currentRepetition >= cycle.repetitions) {
                    currentRepetition = 0;
                    cycleIndex++;
                }
            }
        }
    }
}

void LedController::debugPrintCycle(const LedCycle& cycle) {
    Serial.print(F("Cycle added: On: "));
    Serial.print(cycle.durationOn);
    Serial.print(F(", Off: "));
    Serial.print(cycle.durationOff);
    Serial.print(F(", Repetitions: "));
    Serial.println(cycle.repetitions);
}

void LedController::changeLedStatus(bool status) {
    if (ledStatus != status) {
        ledStatus = status;
        Serial.print(F("LED "));
        Serial.println(status ? F("On") : F("Off"));
        // Implement actual LED change here, e.g., digitalWrite(LED_PIN, ledStatus ? HIGH : LOW);
        digitalWrite(D0, ledStatus ? HIGH : LOW);
    }
}
