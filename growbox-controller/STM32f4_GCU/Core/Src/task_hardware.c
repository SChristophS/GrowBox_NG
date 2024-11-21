/* task_hardware.c */

#include "task_hardware.h"
#include "cmsis_os.h"
#include "hardware.h"
#include "uart_redirect.h"
#include "main.h"
#include "globals.h"
#include <stdio.h>
#include "message_types.h"
#include "logger.h"
#include "task_network.h"
#include "helper_websocket.h"


extern osMessageQueueId_t xHardwareQueueHandle;

void StartHardwareTask(void *argument)
{
    LOG_INFO("task_hardware.c: Starting Hardware Task");

    // Initialisiere den PWM-Timer
    if (HAL_TIM_PWM_Start(&LED_DIM_TIM, LED_DIM_CHANNEL) != HAL_OK) {
        LOG_ERROR("task_hardware.c: Error! Unable to start LED Timer");
        Error_Handler();
    }

    HardwareCommand cmd;
    LOG_DEBUG("task_hardware.c: Size of HardwareCommand: %lu bytes", (unsigned long)sizeof(HardwareCommand));

    for (;;)
    {
        LOG_DEBUG("task_hardware.c: Checking if new command is available");
        if (osMessageQueueGet(xHardwareQueueHandle, &cmd, NULL, osWaitForever) == osOK) {
            LOG_INFO("task_hardware.c: New command received");
            // Check if commandType is a string or needs conversion
            // Example for converting if cmd.commandType is an enum:
            const char *commandTypeString = CommandTypeToString(cmd.commandType); // Function to convert enum to string
            LOG_INFO("task_hardware.c: Command type received: %s", commandTypeString);


            switch (cmd.commandType) {
                case COMMAND_CONTROL_PUMP:
                    LOG_INFO("task_hardware.c: Received COMMAND_CONTROL_PUMP for Pump %d, Enable: %s", cmd.deviceId, cmd.enable ? "true" : "false");
                    if (cmd.enable) {
                        LOG_DEBUG("task_hardware.c: Activating pump with device ID %d", cmd.deviceId);
                        EnablePump(cmd.deviceId);
                    } else {
                        LOG_DEBUG("task_hardware.c: Deactivating pump with device ID %d", cmd.deviceId);
                        DisablePump(cmd.deviceId);
                        break;
                    }
                    break;

                case COMMAND_CONTROL_LIGHT:
                    LOG_INFO("task_hardware.c: Setting Light Intensity to %d", cmd.intensity);

                    if (SetLightIntensity(cmd.intensity) != HAL_OK) {
                        LOG_WARN("task_hardware.c: Failed to set light intensity to %d", cmd.intensity);
                        break;
                    }
                    send_status_update(MESSAGE_TYPE_STATUS_UPDATE, DEVICE_CONTROLLER, TARGET_LIGHT_INTENSITY, cmd.intensity);
                    break;


                default:
                    LOG_WARN("task_hardware.c: Unknown command received");
                    break;
            }
        } else {
            LOG_ERROR("task_hardware.c: Failed to retrieve message from queue");
        }
    }
}



