/* task_sensor.c */

#include "task_sensor.h"
#include "controller_state.h"
#include "cmsis_os.h"
#include "uart_redirect.h"
#include "hardware.h"
#include <stdio.h>
#include <stdbool.h>
#include "globals.h"
#include "helper_websocket.h"
#include "message_types.h"
#include "logger.h"
#include "task_network.h"
#include "message_types.h"

#define SENSOR_TASK_PERIOD_MS 500 // Leseintervall in Millisekunden


void StartSensorTask(void *argument)
{
    printf("task_sensor.c: Starting Sensor Task\n");

    for (;;)
    {
        // Sensorwerte lesen
        bool currentSensorOben = ReadSensorOben();
        bool currentSensorUnten = ReadSensorUnten();

        //printf("task_sensor.c: currentSensorOben: %i\r\n", currentSensorOben);
        //printf("task_sensor.c: currentSensorUnten: %i\r\n", currentSensorUnten);

        bool WaterSensorObenValueChanged = false;
        bool WaterSensorUntenValueChanged = false;
        bool WaterSensorValueChanged = false;

        osMutexAcquire(gControllerStateMutexHandle, osWaitForever);

        // Überprüfe, ob sich die Sensorwerte geändert haben
        if (gControllerState.sensorOben != currentSensorOben) {
            gControllerState.sensorOben = currentSensorOben;
            WaterSensorObenValueChanged = true;
            WaterSensorValueChanged = true;
        }

        if (gControllerState.sensorUnten != currentSensorUnten) {
            gControllerState.sensorUnten = currentSensorUnten;
            WaterSensorUntenValueChanged = true;
            WaterSensorValueChanged = true;
        }

        osMutexRelease(gControllerStateMutexHandle);

        if (WaterSensorValueChanged) {
            LOG_DEBUG("task_sensor.c: Water sensor values changed");
            LOG_DEBUG("process_websocket_messages: currentSensorOben: %i, currentSensorUnten: %i", currentSensorOben, currentSensorUnten);
            //printf("task_sensor.c: currentSensorOben: %i\r\n", currentSensorOben);
            //printf("task_sensor.c: currentSensorUnten: %i\r\n", currentSensorUnten);

            if (WaterSensorObenValueChanged){
            	// Sende Statusnachricht für sensorOben
                send_status_update(MESSAGE_TYPE_STATUS_UPDATE, DEVICE_CONTROLLER, HARDWARE_SENSOR_OBEN, currentSensorOben);
            }

            if (WaterSensorUntenValueChanged){
            	// Sende Statusnachricht für sensorUnten
            	send_status_update(MESSAGE_TYPE_STATUS_UPDATE, DEVICE_CONTROLLER, HARDWARE_SENSOR_UNTEN, currentSensorUnten);
            }

            // Setze Event, um andere Tasks zu benachrichtigen
            osEventFlagsSet(gControllerEventGroupHandle, WATER_SENSOR_VALUES_CHANGED_BIT);
        }

        osDelay(SENSOR_TASK_PERIOD_MS);
    }
}
