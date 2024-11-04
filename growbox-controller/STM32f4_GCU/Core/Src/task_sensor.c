/* task_sensor.c */

#include "task_sensor.h"
#include "controller_state.h"
#include "cmsis_os.h"
#include "uart_redirect.h"
#include "hardware.h"
#include <stdio.h>
#include <stdbool.h>
#include "globals.h"

#define SENSOR_TASK_PERIOD_MS 500 // Leseintervall in Millisekunden

void StartSensorTask(void *argument)
{
    printf("task_sensor.c: Starting Sensor Task\n");

    for (;;)
    {
        // Sensorwerte lesen
        bool currentSensorOben = ReadSensorOben();
        bool currentSensorUnten = ReadSensorUnten();

        printf("task_sensor.c: currentSensorOben: %i\r\n", currentSensorOben);
        printf("task_sensor.c: currentSensorUnten: %i\r\n", currentSensorUnten);

        bool WaterSensorValueChanged = false;

        osMutexAcquire(gControllerStateMutexHandle, osWaitForever);

        // Überprüfe, ob sich die Sensorwerte geändert haben
        if (gControllerState.sensorOben != currentSensorOben) {
            gControllerState.sensorOben = currentSensorOben;
            WaterSensorValueChanged = true;
        }

        if (gControllerState.sensorUnten != currentSensorUnten) {
            gControllerState.sensorUnten = currentSensorUnten;
            WaterSensorValueChanged = true;
        }

        osMutexRelease(gControllerStateMutexHandle);

        if (WaterSensorValueChanged) {
            printf("task_sensor.c: Water sensor values changed\r\n");

            // Setze Event, um andere Tasks zu benachrichtigen
            osEventFlagsSet(gControllerEventGroupHandle, WATER_SENSOR_VALUES_CHANGED_BIT);
        }

        osDelay(SENSOR_TASK_PERIOD_MS);
    }
}
