/* task_light_controller.c */

#include "task_light_controller.h"
#include "controller_state.h"
#include "cmsis_os.h"
#include "uart_redirect.h"
#include "hardware.h"
#include "task_state_manager.h"
#include "task_hardware.h" // Für HardwareCommand und HardwareCommandType
#include <stdio.h>
#include "globals.h"
#include <string.h>

typedef enum {
    LIGHT_OFF,
    LIGHT_ON
} LightState;

void UpdateLightControllerState(uint8_t lightIntensity) {
    printf("task_light_controller.c: Update LightIntensity to: %d\r\n", lightIntensity);

    osMutexAcquire(gControllerStateMutexHandle, osWaitForever);

    if (gControllerState.lightIntensity != lightIntensity) {
        gControllerState.lightIntensity = lightIntensity;
        printf("task_light_controller.c: Changed lightIntensity in gControllerState to %d\r\n", lightIntensity);

        osEventFlagsSet(gControllerEventGroupHandle, LIGHT_INTENSITY_CHANGED_BIT);
    }
    osMutexRelease(gControllerStateMutexHandle);
}

void ControlLight(uint8_t lightIntensity) {
    printf("task_light_controller.c: Sending new PWM value to Hardware Task\r\n");

    // Erstelle Hardwarebefehl
    HardwareCommand cmd;
    cmd.commandType = COMMAND_CONTROL_LIGHT;
    cmd.intensity = lightIntensity;

    // Sende Befehl an Hardware-Task
    osMessageQueuePut(xHardwareQueueHandle, &cmd, 0, 0);
}

void StartLightTask(void *argument)
{
    printf("task_light_controller.c: Starting Light Control Task\r\n");

    // Variablen für die Lichtsteuerung
    GrowCycleConfig growConfig;
    bool configLoaded = false;
    bool finished = false;
    LightState currentState = LIGHT_OFF;
    uint32_t phaseStartTime = 0;
    uint32_t phaseDuration = 0;

    uint8_t scheduleIndex = 0;
    uint8_t repetitionIndex = 0;
    uint8_t lightIntensity = 0;

    // Warte auf das Event-Flag, das anzeigt, dass die Konfiguration verfügbar ist
    printf("task_light_controller.c: Waiting for GrowCycleConfig to become available...\r\n");
    osEventFlagsWait(gControllerEventGroupHandle, NEW_GROW_CYCLE_CONFIG_AVAILABLE, osFlagsWaitAny, osWaitForever);

    // Lade die Konfiguration aus der globalen Variable
    osMutexAcquire(gGrowCycleConfigMutexHandle, osWaitForever);
    memcpy(&growConfig, &gGrowCycleConfig, sizeof(GrowCycleConfig));
    osMutexRelease(gGrowCycleConfigMutexHandle);
    configLoaded = true;
    printf("task_light_controller.c: GrowCycleConfig loaded\r\n");

    for(;;)
    {
        // Verarbeite die Lichtzeitpläne
        if (configLoaded && !finished) {
            if (scheduleIndex < growConfig.ledScheduleCount) {
                LedSchedule *schedule = &growConfig.ledSchedules[scheduleIndex];

                if (repetitionIndex < schedule->repetition) {
                    // Starten der Phase, falls nicht bereits laufend
                    if (phaseDuration == 0) {
                        if (currentState == LIGHT_OFF) {
                            // Licht einschalten
                            printf("task_light_controller.c: Turning light ON (Schedule %d, Repetition %d)\r\n", scheduleIndex, repetitionIndex);
                            lightIntensity = 100; // Maximale Intensität
                            UpdateLightControllerState(lightIntensity);
                            ControlLight(lightIntensity);
                            currentState = LIGHT_ON;
                            phaseDuration = schedule->durationOn * 1000; // in Millisekunden
                        } else {
                            // Licht ausschalten
                            printf("task_light_controller.c: Turning light OFF (Schedule %d, Repetition %d)\r\n", scheduleIndex, repetitionIndex);
                            lightIntensity = 0;
                            UpdateLightControllerState(lightIntensity);
                            ControlLight(lightIntensity);
                            currentState = LIGHT_OFF;
                            phaseDuration = schedule->durationOff * 1000; // in Millisekunden
                            repetitionIndex++;
                        }
                        phaseStartTime = osKernelGetTickCount();
                    } else {
                        // Überprüfe, ob die Phase abgeschlossen ist
                        uint32_t elapsedTime = osKernelGetTickCount() - phaseStartTime;
                        if (elapsedTime >= phaseDuration) {
                            // Beende die aktuelle Phase
                            phaseDuration = 0; // Bereit für die nächste Phase
                        }
                    }
                } else {
                    // Nächster Zeitplan
                    scheduleIndex++;
                    repetitionIndex = 0;
                    currentState = LIGHT_OFF;
                    phaseDuration = 0;
                }
            } else {
                // Alle Zeitpläne abgeschlossen
                printf("task_light_controller.c: All LED schedules completed\r\n");
                finished = true;

                // Licht sicherheitshalber ausschalten
                lightIntensity = 0;
                UpdateLightControllerState(lightIntensity);
                ControlLight(lightIntensity);
            }
        } else if (finished) {
            // Warten auf neue Konfiguration
            printf("task_light_controller.c: Waiting for new GrowCycleConfig...\r\n");
            osEventFlagsWait(gControllerEventGroupHandle, NEW_GROW_CYCLE_CONFIG_AVAILABLE, osFlagsWaitAny, osWaitForever);

            // Neue Konfiguration laden
            osMutexAcquire(gGrowCycleConfigMutexHandle, osWaitForever);
            memcpy(&growConfig, &gGrowCycleConfig, sizeof(GrowCycleConfig));
            osMutexRelease(gGrowCycleConfigMutexHandle);
            configLoaded = true;
            finished = false;
            scheduleIndex = 0;
            repetitionIndex = 0;
            currentState = LIGHT_OFF;
            phaseDuration = 0;
            printf("task_light_controller.c: New GrowCycleConfig loaded\r\n");
        }

        // Wartezeit zwischen den Überprüfungen
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
