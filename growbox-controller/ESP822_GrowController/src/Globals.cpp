// Globals.cpp
#include "Globals.h"

bool LedStatus = false;
bool WaterStatus = false;
int TargetTemp = 0;
bool GrowIsRunning = false;


bool shouldConnectWebSocket = false;
bool isWebSocketConnected = false;
String globalChipId;