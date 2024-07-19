#include "cmsis_os.h"
#include "controller_state.h"
#include "uart_redirect.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "websocket_codes.h"
#include "task_network.h"

void add_message_to_websocket_queue(uint8_t target, bool value);




void StartWatcherTask(void *argument) {
    /* USER CODE BEGIN StartWatcherTask */
    /* Infinite loop */
    for (;;) {
    	while (1) {

    	        // Warte auf ein Event
    	        uint32_t eventFlags = osEventFlagsWait(gControllerEventGroup,
    	            WATER_STATE_CHANGED_BIT | PUMP_ZULAUF_STATE_CHANGED_BIT |
    	            PUMP_ABLAUF_STATE_CHANGED_BIT | SENSOR_VOLL_STATE_CHANGED_BIT |
    	            SENSOR_LEER_STATE_CHANGED_BIT | READY_FOR_AUTORUN_STATE_CHANGED_BIT |
					LIGHT_INTESITY_CHANGED_BIT, osFlagsWaitAny, osWaitForever);


    	        if (eventFlags & WATER_STATE_CHANGED_BIT) {
    	            // Wasserzustand hat sich geändert
    	            printf("task_watcher.c: Wasserzustand hat sich geaendert\r\n");

    	            // Nachricht in die entsprechende Queue stellen
    	            osMutexAcquire(gControllerStateMutex, osWaitForever);
    	            bool wasserbeckenZustand = gControllerState.wasserbeckenZustand;
    	            osMutexRelease(gControllerStateMutex);

    	            if (osMessageQueuePut(xWaterControllerQueueHandle, &wasserbeckenZustand, 0, 0) != osOK) {
    	                printf("task_watcher.c: Failed to send message to WaterControllerQueue.\r\n");
    	            } else {
    	                printf("task_watcher.c: Queue hat neuen Zustand erhalten => WaterControllerQueue.\r\n");
    	            }

    	            // Nachricht für WebSocket
                    add_message_to_websocket_queue(CODE_WASSERBECKENZUSTAND, wasserbeckenZustand);
    	        }

    	        if (eventFlags & LIGHT_INTESITY_CHANGED_BIT) {
    	            // Zustand von Sensor Leer hat sich geändert
    	            printf("task_watcher.c: Zustand lightIntensity hat sich geaendert\r\n");

    	            // Nachricht in die entsprechende Queue stellen
    	            osMutexAcquire(gControllerStateMutex, osWaitForever);
    	            int lightIntensity = gControllerState.lightIntensity;
    	            osMutexRelease(gControllerStateMutex);

    	            printf("task_watcher.c: Neue lightIntensity = %d\r\n", lightIntensity);

    	            if (osMessageQueuePut(xLightControllerQueueHandle, &lightIntensity, 0, 0) != osOK) {
    	                printf("task_watcher.c: Failed to send message to xLightControllerQueueHandle.\r\n");
    	            } else {
    	                printf("task_watcher.c: Queue hat neuen Wert %d erhalten => xLightControllerQueueHandle.\r\n", lightIntensity);
    	            }

    	            // Nachricht für WebSocket
    	            add_message_to_websocket_queue(CODE_LIGHT_INTENSITY, lightIntensity);
    	        }



    	        if (eventFlags & READY_FOR_AUTORUN_STATE_CHANGED_BIT) {
    	            // Zustand von Pumpe 1 hat sich geändert
    	            printf("task_watcher.c:	Zustand von READY FOR AUTORUN hat sich geaendert\r\n");

					// Nachricht in die entsprechende Queue stellen
    	            osMutexAcquire(gControllerStateMutex, osWaitForever);
    	            bool readyForAutorun= gControllerState.readyForAutoRun;
    	            osMutexRelease(gControllerStateMutex);

    	            printf("task_watcher.c:	Neuer readyForAutorun = %d\r\n", readyForAutorun);

					if (osMessageQueuePut(xAutoGrowQueueHandle, &readyForAutorun, 0, 0) != osOK) {
						printf("task_watcher.c:	Failed to send message to xAutoGrowQueueHandle.\r\n");
					} else {
						printf("task_watcher.c:	Queue hat neuen Wert %d erhalten => xAutoGrowQueueHandle.\r\n", readyForAutorun);
						add_message_to_websocket_queue(CODE_READY_FOR_AUTORUN, readyForAutorun);
					}
    	        }

    	        if (eventFlags & PUMP_ZULAUF_STATE_CHANGED_BIT) {
    	            // Zustand von Pumpe 1 hat sich geändert
    	            printf("task_watcher.c:	Zustand von Pumpe Zulauf hat sich geaendert\r\n");
    	            // Nachricht in die entsprechende Queue stellen
    	        }
    	        if (eventFlags & PUMP_ABLAUF_STATE_CHANGED_BIT) {
    	            // Zustand von Pumpe 2 hat sich geändert
    	            printf("task_watcher.c:	Zustand von Pumpe Ablauf hat sich geaendert\r\n");
    	            // Nachricht in die entsprechende Queue stellen
    	        }
    	        if (eventFlags & SENSOR_VOLL_STATE_CHANGED_BIT) {
    	            // Zustand von Sensor Voll hat sich geändert
    	            printf("task_watcher.c:	Zustand von Sensor Voll hat sich geaendert\r\n");
    	            // Nachricht in die entsprechende Queue stellen
    	        }
    	        if (eventFlags & SENSOR_LEER_STATE_CHANGED_BIT) {
    	            // Zustand von Sensor Leer hat sich geändert
    	            printf("task_watcher.c:	Zustand von Sensor Leer hat sich geaendert\r\n");
    	            // Nachricht in die entsprechende Queue stellen
    	        }
    	    }
    }
    /* USER CODE END StartWatcherTask */
}


#include "websocket_codes.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <stdbool.h>

extern osMessageQueueId_t xWebSocketQueueHandle;

void add_message_to_websocket_queue(uint8_t target, bool value) {
    printf("task_watcher.c: I add the target %u with the value %d\r\n", target, value);

    // erstelle Nachricht
    MessageForWebSocket msg;

    // Nachricht zuweisen
    msg.target = target;
    msg.value = value;

    // Nachricht in die Queue stellen
    if (osMessageQueuePut(xWebSocketQueueHandle, &msg, 0, 0) != osOK) {
        printf("task_watcher.c: Failed to send message to WebSocketQueue.\r\n");
    } else {
        printf("task_watcher.c: WebSocketQueue hat neue Nachricht erhalten: target=%u, value=%d\r\n", msg.target, msg.value);
    }
}




