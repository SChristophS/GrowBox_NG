/* task_light_controller.c */

#include <state_manager.h>
#include "task_light_controller.h"
#include "controller_state.h"
#include "cmsis_os.h"
#include "uart_redirect.h"
#include "hardware.h"
#include "task_hardware.h" // Für HardwareCommand und HardwareCommandType
#include <stdio.h>
#include "globals.h"
#include <string.h>

// Funktionsprototypen
void UpdateLightControllerState(uint8_t lightIntensity);
void ControlLight(uint8_t lightIntensity);
void achieve_light_intensity(uint8_t intensity);

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

    // Variablen für manuelle Befehle
    LightCommand receivedCommand;
    bool manualOverride = false;
    uint8_t manualDesiredIntensity = 0;


    // Versuchen, initial eine Konfiguration zu laden, falls verfügbar
    if (osMutexAcquire(gGrowCycleConfigMutexHandle, 100) == osOK) {
        memcpy(&growConfig, &gGrowCycleConfig, sizeof(GrowCycleConfig));
        osMutexRelease(gGrowCycleConfigMutexHandle);
        configLoaded = true;
        printf("task_light_controller.c: GrowCycleConfig loaded at startup\r\n");
    } else {
        printf("task_light_controller.c: No GrowCycleConfig available at startup\r\n");
    }

    for(;;)
    {
        // Überprüfe, ob ein manueller Befehl vorliegt
        if (osMessageQueueGet(xLightCommandQueueHandle, &receivedCommand, NULL, 0) == osOK) {
            if (receivedCommand.commandType == LIGHT_COMMAND_SET_INTENSITY) {
                manualOverride = true;
                manualDesiredIntensity = receivedCommand.intensity;
                printf("task_light_controller.c: Received manual light command: Set intensity to %d\r\n", manualDesiredIntensity);
            }
        }

        // Nicht-blockierendes Warten auf neue Konfiguration
        uint32_t flags = osEventFlagsWait(gControllerEventGroupHandle, NEW_GROW_CYCLE_CONFIG_AVAILABLE, osFlagsWaitAny | osFlagsNoClear, 0);
        if (flags & NEW_GROW_CYCLE_CONFIG_AVAILABLE) {

            // Neue Konfiguration local speichern
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
        } else {
            // Keine neue Konfiguration verfügbar, kurze Verzögerung und nächste Iteration
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        // Überprüfen, ob eine Konfiguration geladen ist
        if (!configLoaded || finished) {
        	printf("task_light_controller.c: No CONFIG OR FINISHED \r\n");
        }


        if (manualOverride) {
            printf("task_light_controller.c: Manual override activated\r\n");
            // Manuelle Steuerung
            achieve_light_intensity(manualDesiredIntensity);
            manualOverride = false;
            printf("task_light_controller.c: Manual override deactivated\r\n");
            // Kurze Verzögerung vor der nächsten Iteration
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }


        //printf("task_light_controller.c: gAutomaticModeHandle = %i\r\n", automaticMode);

        // Überprüfen, ob Automatikmodus aktiv ist
        osMutexAcquire(gAutomaticModeHandle, osWaitForever);
        bool isAutomaticMode = automaticMode; // Lokale Kopie erstellen
        osMutexRelease(gAutomaticModeHandle);
        printf("task_light_controller.c: Manual override deactivated\r\n");
        printf("task_light_controller.c: gAutomaticModeHandle = %b\r\n", automaticMode);

        if (!isAutomaticMode) {
            printf("task_light_controller.c: Automatic mode is disabled. Turning off light.\r\n");
            lightIntensity = 0;
            UpdateLightControllerState(lightIntensity);
            ControlLight(lightIntensity);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }


        // Verarbeite die Lichtzeitpläne
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

        // Kurze Verzögerung vor der nächsten Iteration
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void achieve_light_intensity(uint8_t intensity)
{
    printf("task_light_controller.c: Setting light intensity to %d\r\n", intensity);
    UpdateLightControllerState(intensity);
    ControlLight(intensity);
}

void UpdateLightControllerState(uint8_t lightIntensity) {
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
