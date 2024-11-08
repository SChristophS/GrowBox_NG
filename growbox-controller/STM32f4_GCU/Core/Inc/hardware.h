/* hardware.h */

#ifndef HARDWARE_H
#define HARDWARE_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

extern TIM_HandleTypeDef htim10;

#define PUMP_ZULAUF 1
#define PUMP_ABLAUF 2

// PUMP2 ist ZULAUF
// PUMP1 ist ABLAUF

void EnablePump(uint8_t pumpId);
void DisablePump(uint8_t pumpId);
void SetLightIntensity(uint8_t intensity);

bool ReadSensorOben(void);
bool ReadSensorUnten(void);

#endif /* HARDWARE_H */
