/* task_water_controller.h */

#ifndef TASK_WATER_CONTROLLER_H
#define TASK_WATER_CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>

void StartWaterControllerTask(void *argument);
void UpdatePumpState(bool enable, uint8_t pumpId);
void ControlPump(bool enable, uint8_t pumpId);

#endif /* TASK_WATER_CONTROLLER_H */
