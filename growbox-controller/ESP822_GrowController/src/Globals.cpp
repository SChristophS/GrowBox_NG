// Globals.cpp
#include "Globals.h"

// Status of Growbox Cycle
bool LedStatus = false;
bool WaterStatus = false;
int TargetTemp = 0;
bool GrowIsRunning = false;
bool LEDstatus = false;

// chipID
String globalChipId;

// websocket
bool shouldConnectWebSocket = false;
bool isWebSocketConnected = false;



// PIN Mapping
int LEDinternal_PIN = 2; 
//int LED_PIN = 8;
int LED_PIN = 5;