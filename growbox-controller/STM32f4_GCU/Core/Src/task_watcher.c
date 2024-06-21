#include "cmsis_os.h"
#include "controller_state.h"
#include "uart_redirect.h"
#include <stdio.h>

void StartWatcherTask(void *argument) {
    /* USER CODE BEGIN StartWatcherTask */
    /* Infinite loop */
    for (;;) {
    	while (1) {
    	        // Warte auf ein Event
    	        uint32_t eventFlags = osEventFlagsWait(gControllerEventGroup,
    	            WATER_STATE_CHANGED_BIT | PUMP_ZULAUF_STATE_CHANGED_BIT |
    	            PUMP_ABLAUF_STATE_CHANGED_BIT | SENSOR_VOLL_STATE_CHANGED_BIT |
    	            SENSOR_LEER_STATE_CHANGED_BIT, osFlagsWaitAny, osWaitForever);


    	        if (eventFlags & WATER_STATE_CHANGED_BIT) {
    	            // Wasserzustand hat sich geändert
    	            printf("task_watcher.c:	Wasserzustand hat sich geaendert\r\n");

    	            // Nachricht in die entsprechende Queue stellen
    	            osMutexAcquire(gControllerStateMutex, osWaitForever);
    	            bool wasserbeckenZustand = gControllerState.wasserbeckenZustand;
    	            osMutexRelease(gControllerStateMutex);

    	            if (osMessageQueuePut(xWaterControllerQueueHandle, &wasserbeckenZustand, 0, 0) != osOK) {
    	                printf("task_watcher.c:	Failed to send message to WaterControllerQueue.\r\n");
    	            } else {
    	            	printf("task_watcher.c:	Queue hat neuen Zustand erhalten => WaterControllerQueue.\r\n");
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
