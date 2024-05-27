#ifndef LEDCONTROLLER_H
#define LEDCONTROLLER_H

#include <ArduinoJson.h>
#include <vector>

struct LedCycle {
    int durationOn;
    int durationOff;
    int repetitions;
};

class LedController {
public:
    LedController();
    void parseJson(const char* json);
    void update();

private:
    std::vector<LedCycle> cycles;
    unsigned long previousMillis = 0;
    size_t cycleIndex = 0;
    int currentRepetition = 0;
    int currentStateTime = 0;
    
    void debugPrintCycle(const LedCycle& cycle);
};

#endif
