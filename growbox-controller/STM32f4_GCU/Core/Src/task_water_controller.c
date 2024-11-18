/* task_water_controller.c */

#include <state_manager.h>
#include "task_water_controller.h"
#include "controller_state.h"
#include "cmsis_os.h"
#include "uart_redirect.h"
#include "hardware.h"
#include "schedules.h"
#include <stdio.h>
#include "globals.h"
#include "task_hardware.h"
#include <string.h>
#include "logger.h"

extern osMessageQueueId_t xWaterCommandQueueHandle;

#define MAX_FILL_DURATION_MS 300000  // 5 Minuten
#define MAX_EMPTY_DURATION_MS 300000 // 5 Minuten


// Neue Funktionen zum Aktualisieren des Controller-Zustands
void UpdatePumpState(bool enable, uint8_t pumpId) {
    printf("task_water_controller.c: UpdatePumpState - PumpID %d, Enable: %s\r\n", pumpId, enable ? "true" : "false");

    osMutexAcquire(gControllerStateMutexHandle, osWaitForever);

    if (pumpId == PUMP_ZULAUF) {
        if (gControllerState.pumpeZulauf != enable) {
            gControllerState.pumpeZulauf = enable;
            printf("task_water_controller.c: Updated pumpeZulauf to %s\r\n", enable ? "true" : "false");

            // Optional: Event-Flag setzen, falls benötigt
            // osEventFlagsSet(gControllerEventGroupHandle, PUMPE_ZULAUF_CHANGED_BIT);
        }
    } else if (pumpId == PUMP_ABLAUF) {
        if (gControllerState.pumpeAblauf != enable) {
            gControllerState.pumpeAblauf = enable;
            printf("task_water_controller.c: Updated pumpeAblauf to %s\r\n", enable ? "true" : "false");

            // Optional: Event-Flag setzen, falls benötigt
            // osEventFlagsSet(gControllerEventGroupHandle, PUMPE_ABLAUF_CHANGED_BIT);
        }
    }

    osMutexRelease(gControllerStateMutexHandle);
}

void ControlPump(bool enable, uint8_t pumpId) {
    printf("task_water_controller.c: ControlPump - PumpID %d, Enable: %s\r\n", pumpId, enable ? "true" : "false");

    // Aktualisiere den Controller-Zustand
    UpdatePumpState(enable, pumpId);

    // Erstelle Hardwarebefehl
    HardwareCommand cmd;
    cmd.commandType = COMMAND_CONTROL_PUMP;
    cmd.deviceId = pumpId;
    cmd.enable = enable;

    // Sende Befehl an Hardware-Task
    osMessageQueuePut(xHardwareQueueHandle, &cmd, 0, 0);
}

void achieve_water_state_full(void)
{
    printf("task_water_controller.c: Starting process to achieve state 'Voll'\r\n");

    // Deaktivieren der Pumpen vor Beginn
    ControlPump(false, PUMP_ZULAUF);
    ControlPump(false, PUMP_ABLAUF);

    // Starte die Pumpe, um das Becken zu füllen
    ControlPump(true, PUMP_ZULAUF);
    ControlPump(false, PUMP_ABLAUF);

    // Warte, bis der obere Sensor aktiviert ist oder ein Timeout eintritt
    uint32_t startTime = osKernelGetTickCount();
    uint32_t timeout = MAX_FILL_DURATION_MS; // Definieren Sie eine angemessene Timeout-Dauer
    while (1) {
        osMutexAcquire(gControllerStateMutexHandle, osWaitForever);
        bool sensorOben = gControllerState.sensorOben;
        osMutexRelease(gControllerStateMutexHandle);

        if (sensorOben) {
            printf("task_water_controller.c: State 'Voll' achieved (sensorOben is true)\r\n");
            break;
        }

        if ((osKernelGetTickCount() - startTime) > timeout) {
            printf("task_water_controller.c: Timeout while trying to achieve state 'Voll'\r\n");
            break;
        }

        osDelay(pdMS_TO_TICKS(100));
    }

    // Pumpe ausschalten
    ControlPump(false, PUMP_ZULAUF);
    printf("task_water_controller.c: Pump turned off after achieving state 'Voll'\r\n");
}

void achieve_water_state_empty(void)
{
    printf("task_water_controller.c: Starting process to achieve state 'Leer'\r\n");

    // Deaktivieren der Pumpen vor Beginn
    ControlPump(false, PUMP_ZULAUF);
    ControlPump(false, PUMP_ABLAUF);

    // Starte die Pumpe, um das Becken zu entleeren
    ControlPump(false, PUMP_ZULAUF);
    ControlPump(true, PUMP_ABLAUF);

    // Warte, bis beide Sensoren deaktiviert sind oder ein Timeout eintritt
    uint32_t startTime = osKernelGetTickCount();
    uint32_t timeout = MAX_EMPTY_DURATION_MS; // Definieren Sie eine angemessene Timeout-Dauer
    while (1) {
        osMutexAcquire(gControllerStateMutexHandle, osWaitForever);
        bool sensorOben = gControllerState.sensorOben;
        bool sensorUnten = gControllerState.sensorUnten;
        osMutexRelease(gControllerStateMutexHandle);

        if (!sensorOben && !sensorUnten) {
            printf("task_water_controller.c: State 'Leer' achieved (both sensors are false)\r\n");
            break;
        }

        if ((osKernelGetTickCount() - startTime) > timeout) {
            printf("task_water_controller.c: Timeout while trying to achieve state 'Leer'\r\n");
            break;
        }

        osDelay(pdMS_TO_TICKS(100));
    }

    // Pumpe ausschalten
    ControlPump(false, PUMP_ABLAUF);
    printf("task_water_controller.c: Pump turned off after achieving state 'Leer'\r\n");
}

void StartWaterControllerTask(void *argument)
{
    printf("task_water_controller.c: Starting Water Controller Task\r\n");

    LOG_INFO("Starting Water Controller Task");

    GrowCycleConfig growConfig;
    bool configLoaded = false;
    bool finished = false;
    WateringPhase currentPhase = PHASE_FULL;

    uint8_t scheduleIndex = 0;
    uint8_t repetitionIndex = 0;
    bool isPumping = false;
    uint32_t phaseStartTime = 0;
    uint32_t maxPhaseDuration = 0;

    // Variablen für manuelle Befehle
    WaterCommand receivedCommand;
    bool manualOverride = false;
    WaterState manualDesiredState;

    // Versuchen, initial eine Konfiguration zu laden, falls verfügbar
    if (osMutexAcquire(gGrowCycleConfigMutexHandle, osWaitForever) == osOK) {
        memcpy(&growConfig, &gGrowCycleConfig, sizeof(GrowCycleConfig));
        osMutexRelease(gGrowCycleConfigMutexHandle);
        configLoaded = true;
        printf("task_water_controller.c: GrowCycleConfig loaded at startup\r\n");
    } else {
        printf("task_water_controller.c: No GrowCycleConfig available at startup\r\n");
    }

    for (;;)
    {
        // Überprüfe, ob ein manueller Befehl vorliegt
        if (osMessageQueueGet(xWaterCommandQueueHandle, &receivedCommand, NULL, 0) == osOK) {
            if (receivedCommand.commandType == WATER_COMMAND_SET_STATE) {
                manualOverride = true;
                manualDesiredState = receivedCommand.desiredState;
                printf("task_water_controller.c: Received manual water command: %s\r\n",
                       manualDesiredState == WATER_STATE_FULL ? "VOLL" : "LEER");
            }
        }

        if (manualOverride) {
            printf("task_water_controller.c: Manual override activated\r\n");
            // Manuelle Steuerung
            if (manualDesiredState == WATER_STATE_FULL) {
                achieve_water_state_full();
            } else if (manualDesiredState == WATER_STATE_EMPTY) {
                achieve_water_state_empty();
            }
            manualOverride = false;
            printf("task_water_controller.c: Manual override deactivated\r\n");
            // Kurze Verzögerung vor der nächsten Iteration
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        // Überprüfen, ob eine Konfiguration geladen ist
        if (!configLoaded || finished) {
            // Nicht-blockierendes Warten auf neue Konfiguration
            uint32_t flags = osEventFlagsWait(gControllerEventGroupHandle, NEW_GROW_CYCLE_CONFIG_AVAILABLE_WATER, osFlagsWaitAny, 0);
            if (flags & NEW_GROW_CYCLE_CONFIG_AVAILABLE_WATER) {
                // Neue Konfiguration laden
                osMutexAcquire(gGrowCycleConfigMutexHandle, osWaitForever);
                memcpy(&growConfig, &gGrowCycleConfig, sizeof(GrowCycleConfig));
                osMutexRelease(gGrowCycleConfigMutexHandle);
                configLoaded = true;
                finished = false;
                scheduleIndex = 0;
                repetitionIndex = 0;
                currentPhase = PHASE_FULL;
                isPumping = false;
                printf("task_water_controller.c: New GrowCycleConfig loaded\r\n");
            } else {
                // Keine neue Konfiguration verfügbar, kurze Verzögerung und nächste Iteration
                vTaskDelay(pdMS_TO_TICKS(100));
                continue;
            }
        }


        // Überprüfen, ob Automatikmodus aktiv ist
        osMutexAcquire(gAutomaticModeHandle, osWaitForever);
        if (!automaticMode) {
            printf("task_water_controller.c: Automatic mode is disabled. Turning Pumps off.\r\n");
            ControlPump(false, PUMP_ZULAUF);
            ControlPump(false, PUMP_ABLAUF);

            osMutexRelease(gAutomaticModeHandle);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        osMutexRelease(gAutomaticModeHandle);


        // Automatischen Zeitplan ausführen
        if (scheduleIndex < growConfig.wateringScheduleCount) {
            WateringSchedule *schedule = &growConfig.wateringSchedules[scheduleIndex];

            // Überprüfe, ob die aktuelle Wiederholung abgeschlossen ist
            if (repetitionIndex < schedule->repetition) {
                // Überprüfe, ob die Pumpen laufen
                if (!isPumping) {
                    // Starte die aktuelle Phase
                    if (currentPhase == PHASE_FULL) {
                        printf("task_water_controller.c: Starting PHASE_FULL (Schedule %d, Repetition %d)\r\n", scheduleIndex, repetitionIndex);
                        ControlPump(true, PUMP_ZULAUF);
                        ControlPump(false, PUMP_ABLAUF);
                        maxPhaseDuration = schedule->duration_full * 1000; // in Millisekunden
                    } else if (currentPhase == PHASE_EMPTY) {
                        printf("task_water_controller.c: Starting PHASE_EMPTY (Schedule %d, Repetition %d)\r\n", scheduleIndex, repetitionIndex);
                        ControlPump(false, PUMP_ZULAUF);
                        ControlPump(true, PUMP_ABLAUF);
                        maxPhaseDuration = schedule->duration_empty * 1000; // in Millisekunden
                    }
                    isPumping = true;
                    phaseStartTime = osKernelGetTickCount();
                }

                // Sensorwerte lesen
                osMutexAcquire(gControllerStateMutexHandle, osWaitForever);
                bool sensorOben = gControllerState.sensorOben;
                bool sensorUnten = gControllerState.sensorUnten;
                osMutexRelease(gControllerStateMutexHandle);

                // Überprüfe, ob der Zielzustand erreicht wurde
                bool targetAchieved = false;
                if (currentPhase == PHASE_FULL) {
                    if (sensorOben) {
                        targetAchieved = true;
                        printf("task_water_controller.c: PHASE_FULL target achieved (sensorOben = true)\r\n");
                    }
                } else if (currentPhase == PHASE_EMPTY) {
                    if (!sensorOben && !sensorUnten) {
                        targetAchieved = true;
                        printf("task_water_controller.c: PHASE_EMPTY target achieved (sensors are false)\r\n");
                    }
                }

                // Überprüfe, ob die maximale Phasendauer überschritten wurde
                uint32_t elapsedTime = osKernelGetTickCount() - phaseStartTime;
                bool durationExceeded = (elapsedTime >= maxPhaseDuration);

                // Wechsle zur nächsten Phase, wenn Ziel erreicht oder Dauer überschritten
                if (targetAchieved || durationExceeded) {
                    // Beende die aktuelle Phase
                    ControlPump(false, PUMP_ZULAUF);
                    ControlPump(false, PUMP_ABLAUF);
                    isPumping = false;

                    // Halte die aktuelle Phase für die festgelegte Dauer aufrecht
                    uint32_t holdDuration = 0;
                    if (currentPhase == PHASE_FULL) {
                        holdDuration = schedule->duration_full * 1000; // in Millisekunden
                    } else if (currentPhase == PHASE_EMPTY) {
                        holdDuration = schedule->duration_empty * 1000; // in Millisekunden
                    }

                    // Haltezeit abwarten
                    uint32_t holdStartTime = osKernelGetTickCount();
                    while ((osKernelGetTickCount() - holdStartTime) < holdDuration) {
                        // Überprüfe, ob der Zielzustand sich geändert hat
                        osMutexAcquire(gControllerStateMutexHandle, osWaitForever);
                        bool newSensorOben = gControllerState.sensorOben;
                        bool newSensorUnten = gControllerState.sensorUnten;
                        osMutexRelease(gControllerStateMutexHandle);

                        if (currentPhase == PHASE_FULL && !newSensorOben) {
                            printf("task_water_controller.c: PHASE_FULL - Target state changed, aborting hold time.\r\n");
                            break;
                        } else if (currentPhase == PHASE_EMPTY && (newSensorOben || newSensorUnten)) {
                            printf("task_water_controller.c: PHASE_EMPTY - Target state changed, aborting hold time.\r\n");
                            break;
                        }

                        // Überprüfe auf manuelle Befehle während der Haltezeit
                        if (osMessageQueueGet(xWaterCommandQueueHandle, &receivedCommand, NULL, 0) == osOK) {
                            if (receivedCommand.commandType == WATER_COMMAND_SET_STATE) {
                                manualOverride = true;
                                manualDesiredState = receivedCommand.desiredState;
                                printf("task_water_controller.c: Received manual water command during hold time: %s\r\n",
                                       manualDesiredState == WATER_STATE_FULL ? "VOLL" : "LEER");
                                break;
                            }
                        }

                        vTaskDelay(pdMS_TO_TICKS(100));
                    }

                    if (manualOverride) {
                        // Wenn während der Haltezeit ein manueller Befehl empfangen wurde, überspringe den Rest der Automatik
                        continue;
                    }

                    // Wechsle zur nächsten Phase oder Wiederholung
                    if (currentPhase == PHASE_FULL) {
                        currentPhase = PHASE_EMPTY;
                    } else {
                        currentPhase = PHASE_FULL;
                        repetitionIndex++;
                    }
                }
            } else {
                // Nächster Zeitplan
                scheduleIndex++;
                repetitionIndex = 0;
                currentPhase = PHASE_FULL;
            }
        } else {
            // Alle Zeitpläne abgeschlossen
            printf("task_water_controller.c: All watering schedules completed\r\n");
            finished = true;

            // Pumpen sicherheitshalber ausschalten
            ControlPump(false, PUMP_ZULAUF);
            ControlPump(false, PUMP_ABLAUF);
        }

        // Kurze Verzögerung vor der nächsten Iteration
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
