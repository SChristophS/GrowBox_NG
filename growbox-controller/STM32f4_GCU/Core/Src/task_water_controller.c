#include "cmsis_os.h"
#include "controller_state.h"
#include "uart_redirect.h"
#include <stdio.h>

void InitControllerState() {
    // Initialisieren Sie den Zustand
    gControllerState.wasserbeckenZustand = false;
    gControllerState.pumpeZulauf = false;
    gControllerState.pumpeAblauf = false;
    gControllerState.sensorVoll = false;
    gControllerState.sensorLeer = false;

    // Erstellen Sie den Mutex
    gControllerStateMutex = osMutexNew(NULL);
    if (gControllerStateMutex == NULL) {
        // Fehlerbehandlung
    }

    // Erstellen Sie die Event-Gruppe
    gControllerEventGroup = osEventFlagsNew(NULL);
    if (gControllerEventGroup == NULL) {
        // Fehlerbehandlung
    }
}

void UpdateControllerState(bool wasserbeckenZustand, bool pumpeZulauf, bool pumpeAblauf, bool sensorVoll, bool sensorLeer) {
    osMutexAcquire(gControllerStateMutex, osWaitForever);

    if (gControllerState.wasserbeckenZustand != wasserbeckenZustand) {
        gControllerState.wasserbeckenZustand = wasserbeckenZustand;
        osEventFlagsSet(gControllerEventGroup, WATER_STATE_CHANGED_BIT);
    }
    if (gControllerState.pumpeZulauf != pumpeZulauf) {
        gControllerState.pumpeZulauf = pumpeZulauf;
        osEventFlagsSet(gControllerEventGroup, PUMP_ZULAUF_STATE_CHANGED_BIT);
    }
    if (gControllerState.pumpeAblauf != pumpeAblauf) {
        gControllerState.pumpeAblauf = pumpeAblauf;
        osEventFlagsSet(gControllerEventGroup, PUMP_ABLAUF_STATE_CHANGED_BIT);
    }
    if (gControllerState.sensorVoll != sensorVoll) {
        gControllerState.sensorVoll = sensorVoll;
        osEventFlagsSet(gControllerEventGroup, SENSOR_VOLL_STATE_CHANGED_BIT);
    }
    if (gControllerState.sensorLeer != sensorLeer) {
        gControllerState.sensorLeer = sensorLeer;
        osEventFlagsSet(gControllerEventGroup, SENSOR_LEER_STATE_CHANGED_BIT);
    }

    osMutexRelease(gControllerStateMutex);
}


void startWaterControllerTask(void *argument)
{
    /* USER CODE BEGIN startWaterControllerTask */
    bool wasserbeckenZustand;

    /* Infinite loop */
    for(;;)
    {
        // Warte auf Nachrichten in der WaterController-Queue
        if (osMessageQueueGet(xWaterControllerQueueHandle, &wasserbeckenZustand, NULL, osWaitForever) == osOK) {
            // Nachricht drucken
            printf("task_water_controller.c:\tNeuer Wasserzustand: %s\r\n", wasserbeckenZustand ? "voll" : "leer");
        }

        osDelay(1); // Kleine Verzögerung, um CPU-Last zu reduzieren
    }
    /* USER CODE END startWaterControllerTask */
}

