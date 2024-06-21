// controller_state.c

#include "controller_state.h"

ControllerState gControllerState;
osMutexId_t gControllerStateMutex;
osEventFlagsId_t gControllerEventGroup;
