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
    bool wasserbeckenZustand; // true = voll, false = leer
    bool pumpeZulauf; // true = an, false = aus
    bool pumpeAblauf; // true = an, false = aus
    bool sensorOben; // true = aktiviert, false = deaktiviert
    bool sensorUnten; // true = aktiviert, false = deaktiviert
    uint8_t lightIntensity;
    bool readyForAutoRun; // true wenn Daten korrekt im EEPROM gespeichert sind
} ControllerState;


#endif /* INC_CONTROLLER_STATE_H_ */
