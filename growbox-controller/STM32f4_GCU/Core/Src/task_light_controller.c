#include "task_light_controller.h"
#include "state_manager.h"
#include "controller_state.h"
#include "cmsis_os.h"
#include "uart_redirect.h"
#include "hardware.h"
#include "task_hardware.h" // Für HardwareCommand und HardwareCommandType
#include "globals.h"
#include "logger.h" // Logger einbinden
#include <stdio.h>
#include <string.h>

// Funktionsprototypen
void UpdateLightControllerState(uint8_t lightIntensity);
void ControlLight(uint8_t lightIntensity);
void achieve_light_intensity(uint8_t intensity);

void StartLightTask(void *argument)
{
    LOG_INFO("task_light_controller:\tStarting Light Control Task");

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

    // Versuchen, initial eine lokale konfiguration zu laden, falls verfügbar
    if (osMutexAcquire(gGrowCycleConfigMutexHandle, 100) == osOK) {
        memcpy(&growConfig, &gGrowCycleConfig, sizeof(GrowCycleConfig));
        osMutexRelease(gGrowCycleConfigMutexHandle);
        configLoaded = true;
        LOG_INFO("task_light_controller:\tGrowCycleConfig loaded at startup");
    } else {
        LOG_WARN("No GrowCycleConfig available at startup");
    }

    for(;;)
    {
        // Überprüfe, ob ein manueller Befehl vorliegt
        if (osMessageQueueGet(xLightCommandQueueHandle, &receivedCommand, NULL, 0) == osOK) {
            if (receivedCommand.commandType == LIGHT_COMMAND_SET_INTENSITY) {
                manualOverride = true;
                manualDesiredIntensity = receivedCommand.intensity;
                LOG_INFO("task_light_controller:\tReceived manual light command: Set intensity to %d", manualDesiredIntensity);
            }
        }


        // Manuelle Übersteuerung
        if (manualOverride) {
            LOG_INFO("task_light_controller:\tManual override activated");
            achieve_light_intensity(manualDesiredIntensity);
            manualOverride = false;
            LOG_INFO("task_light_controller:\tManual override deactivated");
            // Kurze Verzögerung vor der nächsten Iteration
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        // Nicht-blockierendes Warten auf neue Konfiguration
        int32_t flags = osEventFlagsWait(gControllerEventGroupHandle, NEW_GROW_CYCLE_CONFIG_AVAILABLE_LIGHT, osFlagsWaitAny, 0);
        if (flags > 0) {
            if (flags & NEW_GROW_CYCLE_CONFIG_AVAILABLE_LIGHT) {
                // Neue Konfiguration ist verfügbar
                osMutexAcquire(gGrowCycleConfigMutexHandle, osWaitForever);
                memcpy(&growConfig, &gGrowCycleConfig, sizeof(GrowCycleConfig));
                osMutexRelease(gGrowCycleConfigMutexHandle);

                configLoaded = true;
                finished = false;
                scheduleIndex = 0;
                repetitionIndex = 0;
                currentState = LIGHT_OFF;
                phaseDuration = 0;
                LOG_INFO("task_light_controller:\tNew GrowCycleConfig loaded");
            }
        } else {
            // Keine neue Konfiguration oder ein Fehler ist aufgetreten
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        // Überprüfen, ob eine Konfiguration geladen ist
        if (!configLoaded || finished) {
            LOG_DEBUG("task_light_controller:\tNo configuration loaded or schedules are finished");
            // Kurze Verzögerung vor der nächsten Iteration
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }


        // Überprüfen, ob Automatikmodus aktiv ist
        osMutexAcquire(gAutomaticModeHandle, osWaitForever);
        bool isAutomaticMode = automaticMode; // Lokale Kopie erstellen
        osMutexRelease(gAutomaticModeHandle);
        LOG_DEBUG("task_light_controller:\tAutomatic mode status: %s", isAutomaticMode ? "ON" : "OFF");

        if (!isAutomaticMode) {
            LOG_INFO("task_light_controller:\tAutomatic mode is disabled. Turning off light.");
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
                        LOG_INFO("task_light_controller:\tTurning light ON (Schedule %d, Repetition %d)", scheduleIndex, repetitionIndex);
                        lightIntensity = 100; // Maximale Intensität
                        UpdateLightControllerState(lightIntensity);
                        ControlLight(lightIntensity);
                        currentState = LIGHT_ON;
                        phaseDuration = schedule->durationOn * 1000; // in Millisekunden
                    } else {
                        // Licht ausschalten
                        LOG_INFO("task_light_controller:\tTurning light OFF (Schedule %d, Repetition %d)", scheduleIndex, repetitionIndex);
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
                LOG_INFO("task_light_controller:\tMoving to next LED schedule (Index %d)", scheduleIndex);
            }
        } else {
            // Alle Zeitpläne abgeschlossen
            LOG_INFO("task_light_controller:\tAll LED schedules completed");
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
    LOG_DEBUG("task_light_controller:\tSetting light intensity to %d", intensity);
    UpdateLightControllerState(intensity);
    ControlLight(intensity);
}

void UpdateLightControllerState(uint8_t lightIntensity) {
    osMutexAcquire(gControllerStateMutexHandle, osWaitForever);

    if (gControllerState.lightIntensity != lightIntensity) {
        gControllerState.lightIntensity = lightIntensity;
        LOG_DEBUG("task_light_controller:\tUpdated lightIntensity in gControllerState to %d", lightIntensity);

        osEventFlagsSet(gControllerEventGroupHandle, LIGHT_INTENSITY_CHANGED_BIT);
    }
    osMutexRelease(gControllerStateMutexHandle);
}

void ControlLight(uint8_t lightIntensity) {
    LOG_DEBUG("task_light_controller:\tSending new PWM value to Hardware Task");

    // Erstelle Hardwarebefehl
    HardwareCommand cmd;
    cmd.commandType = COMMAND_CONTROL_LIGHT;
    cmd.intensity = lightIntensity;

    // Sende Befehl an Hardware-Task
    osMessageQueuePut(xHardwareQueueHandle, &cmd, 0, 0);
}
