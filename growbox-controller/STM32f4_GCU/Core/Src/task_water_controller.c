/* task_water_controller.c */

#include "task_water_controller.h"
#include "controller_state.h"
#include "cmsis_os.h"
#include "uart_redirect.h"
#include "hardware.h"
#include "task_state_manager.h"
#include "schedules.h"
#include <stdio.h>
#include "globals.h"
#include "task_hardware.h"
#include <string.h>

typedef enum {
    PHASE_FULL,
    PHASE_EMPTY
} WateringPhase;

void ControlPump(bool enable, uint8_t pumpId) {
    printf("task_water_controller.c: ControlPump - PumpID %d, Enable: %s\r\n", pumpId, enable ? "true" : "false");

    // Erstelle Hardwarebefehl
    HardwareCommand cmd;
    cmd.commandType = COMMAND_CONTROL_PUMP;
    cmd.deviceId = pumpId;
    cmd.enable = enable;

    // Sende Befehl an Hardware-Task
    osMessageQueuePut(xHardwareQueueHandle, &cmd, 0, 0);
}

void StartWaterControllerTask(void *argument)
{
    printf("task_water_controller.c: Starting Water Controller Task\r\n");

    GrowCycleConfig growConfig;
    bool configLoaded = false;
    bool finished = false;
    WateringPhase currentPhase = PHASE_FULL;

    uint8_t scheduleIndex = 0;
    uint8_t repetitionIndex = 0;
    bool isPumping = false;
    uint32_t phaseStartTime = 0;
    uint32_t maxPhaseDuration = 0;

    // Warte auf das Event-Flag, das anzeigt, dass die Konfiguration verfügbar ist
    printf("task_water_controller.c: Waiting for GrowCycleConfig to become available...\r\n");
    osEventFlagsWait(gControllerEventGroupHandle, NEW_GROW_CYCLE_CONFIG_AVAILABLE, osFlagsWaitAny, osWaitForever);

    // Lade die Konfiguration aus der globalen Variable
    osMutexAcquire(gGrowCycleConfigMutexHandle, osWaitForever);
    memcpy(&growConfig, &gGrowCycleConfig, sizeof(GrowCycleConfig));
    osMutexRelease(gGrowCycleConfigMutexHandle);
    configLoaded = true;
    printf("task_water_controller.c: GrowCycleConfig loaded\r\n");

    for (;;)
    {
        if (configLoaded && !finished) {
            // Überprüfe, ob es weitere Zeitpläne gibt
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
                        // Ziel: Zustand "voll" erreichen (sensorOben = true)
                        if (sensorOben) {
                            targetAchieved = true;
                            printf("task_water_controller.c: PHASE_FULL target achieved (sensorOben = true)\r\n");
                        }
                    } else if (currentPhase == PHASE_EMPTY) {
                        // Ziel: Zustand "leer" erreichen (sensorOben = false, sensorUnten = false)
                        if (!sensorOben && !sensorUnten) {
                            targetAchieved = true;
                            printf("task_water_controller.c: PHASE_EMPTY target achieved (sensorOben = false, sensorUnten = false)\r\n");
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
                                printf("task_water_controller.c: PHASE_FULL - Zielzustand geändert, abbrechen.\r\n");
                                break;
                            } else if (currentPhase == PHASE_EMPTY && (newSensorOben || newSensorUnten)) {
                                printf("task_water_controller.c: PHASE_EMPTY - Zielzustand geändert, abbrechen.\r\n");
                                break;
                            }

                            vTaskDelay(pdMS_TO_TICKS(100));
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
        } else if (finished) {
            // Warten auf neue Konfiguration
            printf("task_water_controller.c: Waiting for new GrowCycleConfig...\r\n");
            osEventFlagsWait(gControllerEventGroupHandle, NEW_GROW_CYCLE_CONFIG_AVAILABLE, osFlagsWaitAny, osWaitForever);

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
        }

        // Wartezeit zwischen den Überprüfungen
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
