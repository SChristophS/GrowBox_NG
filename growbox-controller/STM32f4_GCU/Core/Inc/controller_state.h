/*
 * controller_state.h
 *
 *  Created on: Jun 21, 2024
 *      Author: chris
 */

#ifndef INC_CONTROLLER_STATE_H_
#define INC_CONTROLLER_STATE_H_

#include <stdbool.h> // Für den Typ bool
#include "cmsis_os.h" // Für osMutexId_t und osEventFlagsId_t

typedef struct {
    char message[200];
} WebSocketMessage;

typedef struct {
    bool wasserbeckenZustand; // true = voll, false = leer
    bool pumpeZulauf; // true = an, false = aus
    bool pumpeAblauf; // true = an, false = aus
    bool sensorVoll; // true = aktiviert, false = deaktiviert
    bool sensorLeer; // true = aktiviert, false = deaktiviert
    uint8_t lightIntensity;
    bool readyForAutoRun; // true wenn Daten korrekt im EEPROM gespeichert sind
} ControllerState;

// Globale Variablen als extern deklarieren
extern ControllerState gControllerState;

extern osMutexId_t gControllerStateMutex;
extern osEventFlagsId_t gControllerEventGroup;
extern osMessageQueueId_t xWaterControllerQueueHandle;
extern osMessageQueueId_t xLightControllerQueueHandle;
extern osMessageQueueId_t xWebSocketQueueHandle;
extern osMessageQueueId_t xAutoGrowQueueHandle;

// Event-Bits für Benachrichtigung
#define WATER_STATE_CHANGED_BIT (1 << 0)
#define PUMP_ZULAUF_STATE_CHANGED_BIT (1 << 1)
#define PUMP_ABLAUF_STATE_CHANGED_BIT (1 << 2)
#define SENSOR_VOLL_STATE_CHANGED_BIT (1 << 3)
#define SENSOR_LEER_STATE_CHANGED_BIT (1 << 4)
#define LIGHT_INTESITY_CHANGED_BIT (1 << 5)
#define READY_FOR_AUTORUN_STATE_CHANGED_BIT (1 << 6)


#endif /* INC_CONTROLLER_STATE_H_ */
