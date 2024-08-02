#include "cmsis_os.h"
#include "controller_state.h"
#include "uart_redirect.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "helper_websocket.h"
#include "task_network.h"
#include "globals.h"



void StartWatcherTask(void *argument) {
    /* USER CODE BEGIN StartWatcherTask */
    /* Infinite loop */
    for (;;) {
        while (1) {


            // Wait for an event
            uint32_t eventFlags = osEventFlagsWait(gControllerEventGroup,
                WATER_STATE_CHANGED_BIT | PUMP_ZULAUF_STATE_CHANGED_BIT |
                PUMP_ABLAUF_STATE_CHANGED_BIT | SENSOR_VOLL_STATE_CHANGED_BIT |
                SENSOR_LEER_STATE_CHANGED_BIT | READY_FOR_AUTORUN_STATE_CHANGED_BIT |
                LIGHT_INTESITY_CHANGED_BIT, osFlagsWaitAny, osWaitForever);

            if (eventFlags & WATER_STATE_CHANGED_BIT) {
                // Water state changed
                printf("task_watcher.c: Wasserzustand hat sich geaendert\r\n");

                // Put message in the appropriate queue

                osMutexAcquire(gControllerStateMutex, osWaitForever);
                uint8_t wasserbeckenZustand = gControllerState.wasserbeckenZustand;
                osMutexRelease(gControllerStateMutex);

                if (osMessageQueuePut(xWaterControllerQueueHandle, &wasserbeckenZustand, 0, 0) != osOK) {
                    printf("task_watcher.c: Failed to send message to WaterControllerQueue.\r\n");
                } else {
                    printf("task_watcher.c: Queue hat neuen Zustand erhalten => WaterControllerQueue.\r\n");
                }

                // Message for WebSocket
                add_message_to_websocket_queue(MESSAGE_TYPE_UPDATE, DEVICE_CONTROLLER, TARGET_WATER_LEVEL, ACTION_UPDATE, wasserbeckenZustand);

            }

            if (eventFlags & LIGHT_INTESITY_CHANGED_BIT) {
                // Light intensity state changed
                printf("task_watcher.c: Zustand lightIntensity hat sich geaendert\r\n");

                // Put message in the appropriate queue
                osMutexAcquire(gControllerStateMutex, osWaitForever);
                uint8_t lightIntensity = gControllerState.lightIntensity;
                osMutexRelease(gControllerStateMutex);
                printf("task_watcher.c: Neue lightIntensity = %d\r\n", lightIntensity);

                if (osMessageQueuePut(xLightControllerQueueHandle, &lightIntensity, 0, 0) != osOK) {
                    printf("task_watcher.c: Failed to send message to xLightControllerQueueHandle.\r\n");
                } else {
                    printf("task_watcher.c: Queue hat neuen Wert %d erhalten => xLightControllerQueueHandle.\r\n", lightIntensity);
                }

                // Message for WebSocket
                add_message_to_websocket_queue(MESSAGE_TYPE_UPDATE, DEVICE_CONTROLLER, TARGET_LIGHT_INTENSITY, ACTION_UPDATE, lightIntensity);
            }

            if (eventFlags & READY_FOR_AUTORUN_STATE_CHANGED_BIT) {
                // Ready for autorun state changed
                printf("task_watcher.c: Zustand von READY FOR AUTORUN hat sich geaendert\r\n");

                // Put message in the appropriate queue
                osMutexAcquire(gControllerStateMutex, osWaitForever);
                bool readyForAutorun = gControllerState.readyForAutoRun;
                osMutexRelease(gControllerStateMutex);

                printf("task_watcher.c: Neuer readyForAutorun = %d\r\n", readyForAutorun);

                if (osMessageQueuePut(xAutoGrowQueueHandle, &readyForAutorun, 0, 0) != osOK) {
                    printf("task_watcher.c: Failed to send message to xAutoGrowQueueHandle.\r\n");
                } else {
                    printf("task_watcher.c: Queue hat neuen Wert %d erhalten => xAutoGrowQueueHandle.\r\n", readyForAutorun);
                    add_message_to_websocket_queue(MESSAGE_TYPE_UPDATE, DEVICE_CONTROLLER, TARGET_READYFORAUTORUN, ACTION_UPDATE, readyForAutorun);
                }
            }

            if (eventFlags & PUMP_ZULAUF_STATE_CHANGED_BIT) {
                // State of Pump 1 changed
                printf("task_watcher.c: Zustand von Pumpe Zulauf hat sich geaendert\r\n");
                // Put message in the appropriate queue
            }

            if (eventFlags & PUMP_ABLAUF_STATE_CHANGED_BIT) {
                // State of Pump 2 changed
                printf("task_watcher.c: Zustand von Pumpe Ablauf hat sich geaendert\r\n");
                // Put message in the appropriate queue
            }

            if (eventFlags & SENSOR_VOLL_STATE_CHANGED_BIT) {
                // State of Sensor Full changed
                printf("task_watcher.c: Zustand von Sensor Voll hat sich geaendert\r\n");
                // Put message in the appropriate queue
            }

            if (eventFlags & SENSOR_LEER_STATE_CHANGED_BIT) {
                // State of Sensor Empty changed
                printf("task_watcher.c: Zustand von Sensor Leer hat sich geaendert\r\n");
                // Put message in the appropriate queue
            }

            // CPU bisschen entlasten damit die anderen Tasks auch genug Leistung haben
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }
    /* USER CODE END StartWatcherTask */
}




