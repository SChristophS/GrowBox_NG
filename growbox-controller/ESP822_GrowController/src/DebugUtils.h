#ifndef DebugUtils_h
#define DebugUtils_h

#include <Arduino.h>

enum DebugLevel {
    DEBUG_NONE = 0, // Keine Debug-Nachrichten
    DEBUG_ERROR,    // Nur Fehlermeldungen
    DEBUG_WARN,     // Warnungen und Fehler
    DEBUG_INFO,     // Informative Nachrichten, Warnungen und Fehler
    DEBUG_VERBOSE   // Alle Nachrichten
};

void debugPrint(DebugLevel level, const String& message);

#endif
