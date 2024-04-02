#include <fstream>
#include <iostream>
#include "Cycle.h"
#include "json.hpp"
#include "Globals.h"

using json = nlohmann::json;

void parseAndExecute(const std::string& filePath) {
    std::ifstream i(filePath);
    if (!i.is_open()) {
        std::cerr << "Fehler beim Öffnen der Datei: " << filePath << std::endl;
        return;
    }

    json j;
    i >> j;

    if (j.contains("growData")) {
        int startFromHere = j["growData"]["startFromHere"];

        GrowCycle ledCycle;
        if (j["growData"].contains("ledCycles")) {
            for (const auto& item : j["growData"]["ledCycles"]) {
                ledCycle.addCycle(item["durationOn"], item["durationOff"], item["ledRepetitions"]);
            }
            ledCycle.executeCycle(startFromHere);
        }
    }
}



int main() {
    parseAndExecute("json_example.json");

    if (LedStatus) {
        // Logik, wenn LED an ist
        std::cout << "LED AN" << std::endl;
    } else {
        // Logik, wenn LED aus ist
        std::cout << "LED AUS" << std::endl;
    }

    return 0;
}
