/* task_hardware.h */

#ifndef TASK_HARDWARE_H
#define TASK_HARDWARE_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    COMMAND_CONTROL_PUMP,
    COMMAND_CONTROL_LIGHT,
    // Weitere Befehle
} HardwareCommandType;

typedef struct {
    HardwareCommandType commandType;
    uint8_t deviceId;
    bool enable;
    uint8_t intensity;
} HardwareCommand;

void StartHardwareTask(void *argument);

#endif /* TASK_HARDWARE_H */
