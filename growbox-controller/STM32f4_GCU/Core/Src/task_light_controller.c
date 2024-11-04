/* task_light_controller.c */

#include "task_light_controller.h"
#include "controller_state.h"
#include "cmsis_os.h"
#include "uart_redirect.h"
#include "hardware.h"
#include "task_state_manager.h"
#include "task_hardware.h" // Für HardwareCommand und HardwareCommandType
#include <stdio.h>
#include "main.h"
#include "controller_state.h"
#include "globals.h"


void UpdateLightControllerState(int lightIntensity) {
    printf("task_light_controller.c: Update LightIntensity to: %d\r\n", lightIntensity);

    osMutexAcquire(gControllerStateMutexHandle, osWaitForever);

    if (gControllerState.lightIntensity != lightIntensity) {
        gControllerState.lightIntensity = lightIntensity;
        printf("task_light_controller.c: Changed lightIntensity in gControllerState to %d\r\n", lightIntensity);

        osEventFlagsSet(gControllerEventGroupHandle, LIGHT_INTENSITY_CHANGED_BIT);
    }
    osMutexRelease(gControllerStateMutexHandle);
}

void ControlLight(int lightIntensity) {
    printf("task_light_controller.c: Sending new PWM value to Hardware Task\r\n");

    // Erstelle Hardwarebefehl
    HardwareCommand cmd;
    cmd.commandType = COMMAND_CONTROL_LIGHT;
    cmd.intensity = lightIntensity; // Hier verwenden wir deviceId für die Intensität

    // Sende Befehl an Hardware-Task
    osMessageQueuePut(xHardwareQueueHandle, &cmd, 0, 0);
}

void StartLightTask(void *argument)
{
    printf("task_light_controller.c: Starting Light Control Task\r\n");

    uint8_t lightIntensity = 0;

    // Variablen für die Lichtsteuerung
    GrowCycleConfig growConfig;
    bool configLoaded = false;
    bool finished = false;

    // Warte auf das Event-Flag, das anzeigt, dass die Konfiguration verfügbar ist
    printf("task_light_controller.c: Waiting for GrowCycleConfig to become available...\r\n");
    uint32_t flags = osEventFlagsWait(gControllerEventGroupHandle, NEW_GROW_CYCLE_CONFIG_AVAILABLE, osFlagsWaitAny, osWaitForever);
    printf("task_light_controller.c: waiting done\r\n");

    // Lade die Konfiguration
    if (flags & NEW_GROW_CYCLE_CONFIG_AVAILABLE) {
        if (load_grow_cycle_config(&growConfig)) {
            printf("task_light_controller.c: Successfully loaded GrowCycle from EEPROM\r\n");
            configLoaded = true;
        } else {
            printf("task_light_controller.c: Failed to load grow cycle config\r\n");
        }
    } else {
        printf("task_light_controller.c: Failed to receive new grow cycle config available flag\r\n");
    }

    uint8_t scheduleIndex = 0;
    uint8_t repetitionIndex = 0;
    bool lightOn = false;
    uint32_t delayTime = 0;

    for(;;)
    {
        // Warte auf Nachrichten in der LightController-Queue, mit Timeout
        if (osMessageQueueGet(xLightControllerQueueHandle, &lightIntensity, NULL, 100) == osOK) {
            printf("task_light_controller.c: New lightIntensity value: %d\r\n", lightIntensity);

            // Update Controller State
            UpdateLightControllerState(lightIntensity);
        }

        if (configLoaded) {
        	printf("task_light_controller.c: config is loaded\r\n");
            // Verarbeite die Lichtzeitpläne
            if (scheduleIndex < growConfig.ledScheduleCount) {
                LedSchedule schedule = growConfig.ledSchedules[scheduleIndex];

                if (repetitionIndex < schedule.repetition) {
                    if (!lightOn) {
                        // Licht einschalten
                        printf("task_light_controller.c: Turning light ON (Schedule %d, Repetition %d)\r\n", scheduleIndex, repetitionIndex);
                        UpdateLightControllerState(100);
                        ControlLight(100);
                        lightOn = true;
                        delayTime = schedule.durationOn * 1000; // in Millisekunden
                    } else {
                        // Licht ausschalten
                        printf("task_light_controller.c: Turning light OFF (Schedule %d, Repetition %d)\r\n", scheduleIndex, repetitionIndex);
                        UpdateLightControllerState(0);
                        ControlLight(0);
                        lightOn = false;
                        delayTime = schedule.durationOff * 1000; // in Millisekunden

                        repetitionIndex++;
                    }
                } else {
                    // Nächster Zeitplan
                    scheduleIndex++;
                    repetitionIndex = 0;
                }
            } else {
                // Alle Zeitpläne abgeschlossen
                printf("task_light_controller.c: All LED schedules completed\r\n");
                configLoaded = false;
                finished = true;
            }
        } else {
            // Versuche, die Konfiguration erneut zu laden
            if (load_grow_cycle_config(&growConfig) && !finished) {
                printf("task_light_controller.c: Successfully loaded GrowCycle from EEPROM\r\n");
                configLoaded = true;
                scheduleIndex = 0;
                repetitionIndex = 0;
                lightOn = false;
            } else {
                printf("task_light_controller.c: Failed to load grow cycle config\r\n");
                osDelay(5000); // Warte 5 Sekunden, bevor du es erneut versuchst
            }
        }

        // Wartezeit entsprechend einstellen
        if (delayTime > 0) {
            osDelay(delayTime);
            delayTime = 0;
        } else {
            osDelay(100);
        }
    }
}
