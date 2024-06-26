#include "cmsis_os.h"
#include "controller_state.h"
#include "uart_redirect.h"
#include <stdio.h>
#include "main.h"

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

void UpdateWaterControllerState(bool wasserbeckenZustand, bool pumpeZulauf, bool pumpeAblauf, bool sensorVoll, bool sensorLeer) {
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

void ControlPump(bool enable, GPIO_TypeDef* enablePort, uint16_t enablePin, GPIO_TypeDef* in1Port, uint16_t in1Pin, GPIO_TypeDef* in2Port, uint16_t in2Pin) {
    if (enable) {
        HAL_GPIO_WritePin(in1Port, in1Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(in2Port, in2Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(enablePort, enablePin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(enablePort, enablePin, GPIO_PIN_RESET);
    }
}

void StartWaterControllerTask(void *argument)
{
    /* USER CODE BEGIN startWaterControllerTask */
    bool wasserbeckenZustand;
    GPIO_PinState stateSensorOben, stateSensorUnten;

    /* Infinite loop */
    for(;;)
    {
        stateSensorOben = HAL_GPIO_ReadPin(WATER_OBEN_GPIO_Port, WATER_OBEN_Pin);
        stateSensorUnten = HAL_GPIO_ReadPin(WATER_UNTEN_GPIO_Port, WATER_UNTEN_Pin);

        //printf("task_water_controller.c: Sensor Oben = %s, Unten = %s\r\n", stateSensorOben == GPIO_PIN_SET ? "HIGH" : "LOW", stateSensorUnten == GPIO_PIN_SET ? "HIGH" : "LOW");


        // Warte auf Nachrichten in der WaterController-Queue, mit Timeout
        if (osMessageQueueGet(xWaterControllerQueueHandle, &wasserbeckenZustand, NULL, 100) == osOK) {
            // Nachricht drucken
            printf("task_water_controller.c:\tNeuer Wasserzustand: %s\r\n", wasserbeckenZustand ? "voll" : "leer");

            // Update Controller State
            UpdateWaterControllerState(wasserbeckenZustand, gControllerState.pumpeZulauf, gControllerState.pumpeAblauf, gControllerState.sensorVoll, gControllerState.sensorLeer);
        }


        // Check and control the water tank state
        if (gControllerState.wasserbeckenZustand) {
            // Soll-Zustand: Wassertank voll
        	//printf("Soll-Zustand: Wassertank voll\r\n");


            if (stateSensorOben) {
            	//printf("Sensor oben meldet WASSER ERKANNT\r\n");
            	// Stop Pumping zulauf
            	ControlPump(false, PUMP2_ENABLE_GPIO_Port, PUMP2_ENABLE_Pin, PUMP2_IN1_GPIO_Port, PUMP2_IN1_Pin, PUMP2_IN2_GPIO_Port, PUMP2_IN2_Pin);

            } else {
            	//printf("Sensor oben meldet KEIN WASSER ERKANNT\r\n");

                // Start Pumping zulauf
            	ControlPump(true, PUMP2_ENABLE_GPIO_Port, PUMP2_ENABLE_Pin, PUMP2_IN1_GPIO_Port, PUMP2_IN1_Pin, PUMP2_IN2_GPIO_Port, PUMP2_IN2_Pin);
            }
        } else {
        	//printf("Soll-Zustand: Wassertank leer\r\n");

            // Soll-Zustand: Wassertank leer

            if (stateSensorUnten) {
            	//printf("Sensor UNTEN meldet WASSER ERKANNT\r\n");
                // Start Pumping Ablauf
            	//printf("Run PUMPE ABLAUF\r\n");
                ControlPump(true, PUMP1_ENABLE_GPIO_Port, PUMP1_ENABLE_Pin, PUMP1_IN1_GPIO_Port, PUMP1_IN1_Pin, PUMP1_IN2_GPIO_Port, PUMP1_IN2_Pin);
            } else {
            	//printf("Sensor UNTEN meldet kein WASSER ERKANNT\r\n");
                // Stop Pumping Zulauf

            	ControlPump(false, PUMP1_ENABLE_GPIO_Port, PUMP1_ENABLE_Pin, PUMP1_IN1_GPIO_Port, PUMP1_IN1_Pin, PUMP1_IN2_GPIO_Port, PUMP1_IN2_Pin);
            }
        }

        osDelay(1); // Kleine Verzögerung, um CPU-Last zu reduzieren
    }
    /* USER CODE END startWaterControllerTask */
}
