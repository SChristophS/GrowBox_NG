// Globals.h
#include <stdbool.h>
#include <Arduino.h>

extern String globalChipId; // Deklariere die globale Variable
extern bool shouldConnectWebSocket; // 
extern bool isWebSocketConnected;

extern bool WaterStatus; // false für Empty, true für Full
extern int TargetTemp; // Zieltemperatur in Grad Celsius

extern bool GrowIsRunning;

extern int LEDinternal_PIN;
extern int LED_PIN;

extern bool LEDstatus;