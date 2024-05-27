#include "DebugUtils.h"

extern DebugLevel currentDebugLevel; // Muss im Hauptprogramm definiert werden

void debugPrint(DebugLevel level, const String& message) {
    if (level <= currentDebugLevel) {
        Serial.println(message);
    }
}
