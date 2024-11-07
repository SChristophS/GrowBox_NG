/* task_hardware.c */

#include "task_hardware.h"
#include "cmsis_os.h"
#include "hardware.h"
#include "uart_redirect.h"
#include "main.h"
#include <stdio.h>

extern osMessageQueueId_t xHardwareQueueHandle;

void StartHardwareTask(void *argument)
{
    printf("task_hardware.c: Starting Hardware Task\r\n");

    // Initialisiere den PWM-Timer
    if (HAL_TIM_PWM_Start(&LED_DIM_TIM, LED_DIM_CHANNEL) != HAL_OK) {
        printf("task_hardware.c: Error! Unable to start LED Timer\r\n");
        Error_Handler();
    }

    HardwareCommand cmd;
    printf("task_hardware.c: Size of HardwareCommand: %lu bytes\r\n", (unsigned long)sizeof(HardwareCommand));


    for (;;)
    {
        if (osMessageQueueGet(xHardwareQueueHandle, &cmd, NULL, osWaitForever) == osOK) {
            switch (cmd.commandType) {
                case COMMAND_CONTROL_PUMP:
                	printf("task_hardware.c: received COMMAND_CONTROL_PUMP for Pump %d, Enable: %s\r\n", cmd.deviceId, cmd.enable ? "true" : "false");

                    if (cmd.enable) {
                        // Pumpe einschalten
                    	printf("task_hardware.c: activate pump with device %d\r\n", cmd.deviceId);
                        EnablePump(cmd.deviceId);
                    } else {
                        // Pumpe ausschalten
                    	printf("task_hardware.c: deactivate pump with device %d\r\n", cmd.deviceId);
                        DisablePump(cmd.deviceId);
                    }
                    break;

                case COMMAND_CONTROL_LIGHT:
                    printf("task_hardware.c: Setting Light Intensity to %d\r\n", cmd.intensity);
                    SetLightIntensity(cmd.intensity);
                    break;

                // Weitere Fälle

                default:
                    printf("task_hardware.c: Unknown command\n");
                    break;
            }
        }
    }
}
