// Globals.h
#include <stdbool.h>
#include <Arduino.h>

extern String globalChipId; // Deklariere die globale Variable
extern bool shouldConnectWebSocket; // 
extern bool isWebSocketConnected;

extern bool LedStatus; // false für Off, true für On
extern bool WaterStatus; // false für Empty, true für Full
extern int TargetTemp; // Zieltemperatur in Grad Celsius

extern bool GrowIsRunning;