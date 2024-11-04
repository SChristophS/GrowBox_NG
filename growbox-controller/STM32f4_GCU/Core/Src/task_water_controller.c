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
#include "task_hardware.h" // Für HardwareCommand und HardwareCommandType



void ControlPump(bool enable, uint8_t pumpId) {
    printf("task_water_controller.c: Sending pump control command to Hardware Task for PumpID %i\r\n", pumpId);

    // Erstelle Hardwarebefehl
    HardwareCommand cmd;
    cmd.commandType = COMMAND_CONTROL_PUMP;
    cmd.deviceId = pumpId;
    cmd.enable = enable;
    printf("task_water_controller.c: Size of HardwareCommand: %lu bytes\r\n", (unsigned long)sizeof(HardwareCommand));

    printf("task_water_controller.c: cmd.commandType %i\r\n", cmd.commandType);
    printf("task_water_controller.c: cmd.deviceId %i\r\n", cmd.deviceId);
    printf("task_water_controller.c: cmd.enable %s \r\n", cmd.enable ? "true" : "false");

    // Sende Befehl an Hardware-Task
    osMessageQueuePut(xHardwareQueueHandle, &cmd, 0, 0);
}

void StartWaterControllerTask(void *argument)
{
    printf("task_water_controller.c: Starting Water Controller Task\r\n");

    GrowCycleConfig growConfig;
    bool configLoaded = false;
    bool finished = false;
    uint8_t phase = 1;
    uint32_t startTime = 0;

    // Warte auf das Event-Flag, das anzeigt, dass die Konfiguration verfügbar ist
    printf("task_water_controller.c: Waiting for GrowCycleConfig to become available...\r\n");
    uint32_t flags = osEventFlagsWait(gControllerEventGroupHandle, NEW_GROW_CYCLE_CONFIG_AVAILABLE, osFlagsWaitAny, osWaitForever);
    printf("task_water_controller.c: waiting done\r\n");


    // Lade die Konfiguration
    if (flags & NEW_GROW_CYCLE_CONFIG_AVAILABLE) {
        if (load_grow_cycle_config(&growConfig)) {
            printf("task_water_controller.c: Successfully loaded GrowCycle from EEPROM\r\n");
            configLoaded = true;
        } else {
            printf("task_water_controller.c: Failed to load grow cycle config\r\n");
        }
    } else {
        printf("task_water_controller.c: Failed to receive new grow cycle config available flag\r\n");
    }



    uint8_t scheduleIndex = 0;
    uint8_t repetitionIndex = 0;
    bool pumping = false;
    uint32_t delayTime = 0;

    for (;;)
    {
        // Warte auf Sensorwert-Änderungen
        uint32_t flags = osEventFlagsWait(gControllerEventGroupHandle, WATER_SENSOR_VALUES_CHANGED_BIT, osFlagsWaitAny, 100);

        if (flags & WATER_SENSOR_VALUES_CHANGED_BIT) {
            // Sensorwerte lesen
            osMutexAcquire(gControllerStateMutexHandle, osWaitForever);
            bool sensorOben = gControllerState.sensorOben;
            bool sensorUnten = gControllerState.sensorUnten;
            osMutexRelease(gControllerStateMutexHandle);

            printf("task_water_controller.c: New sensor values! SensorOben=%s, SensorUnten=%s\r\n",
                   sensorOben ? "true" : "false", sensorUnten ? "true" : "false");
        }

        if (configLoaded) {
            // Verarbeite die Bewässerungszeitpläne
            if (scheduleIndex < growConfig.wateringScheduleCount) {
                WateringSchedule schedule = growConfig.wateringSchedules[scheduleIndex];

                if (repetitionIndex < schedule.repetition) {
                    if (!pumping) {
                        // Start der Bewässerung für die aktuelle Phase
                        printf("task_water_controller.c: Starting watering (Schedule %d, Repetition %d, Phase %d)\r\n", scheduleIndex, repetitionIndex, phase);

                        if (phase == 1) {
                            // Phase 1: Zustand "voll" erreichen
                            ControlPump(true, PUMP_ZULAUF);
                            ControlPump(false, PUMP_ABLAUF);
                            delayTime = schedule.duration_full * 1000; // in Millisekunden
                        } else if (phase == 2) {
                            // Phase 2: Zustand "leer" erreichen
                            ControlPump(false, PUMP_ZULAUF);
                            ControlPump(true, PUMP_ABLAUF);
                            delayTime = schedule.duration_empty * 1000; // in Millisekunden
                        }

                        pumping = true;
                        startTime = osKernelGetTickCount();
                    } else {
                        // Überprüfe, ob das Ziel erreicht wurde oder die Zeit abgelaufen ist
                        uint32_t currentTime = osKernelGetTickCount();
                        if ((currentTime - startTime) >= delayTime) {
                            // Maximale Dauer erreicht, Pumpen stoppen
                            ControlPump(false, PUMP_ZULAUF);
                            ControlPump(false, PUMP_ABLAUF);
                            pumping = false;

                            if (phase == 1) {
                                phase = 2; // Wechsle zu Phase 2
                            } else {
                                // Beide Phasen abgeschlossen
                                repetitionIndex++;
                                phase = 1; // Zurück zu Phase 1
                            }
                        } else {
                            // Überprüfe Sensorwerte
                            osMutexAcquire(gControllerStateMutexHandle, osWaitForever);
                            bool sensorOben = gControllerState.sensorOben;
                            bool sensorUnten = gControllerState.sensorUnten;
                            osMutexRelease(gControllerStateMutexHandle);

                            bool targetAchieved = false;

                            if (phase == 1) {
                                // Ziel ist Zustand "voll"
                                if (sensorOben) {
                                    targetAchieved = true;
                                }
                            } else if (phase == 2) {
                                // Ziel ist Zustand "leer"
                                if (!sensorOben && !sensorUnten) {
                                    targetAchieved = true;
                                }
                            }

                            if (targetAchieved) {
                                // Pumpen stoppen
                                ControlPump(false, PUMP_ZULAUF);
                                ControlPump(false, PUMP_ABLAUF);
                                pumping = false;

                                if (phase == 1) {
                                    phase = 2; // Wechsle zu Phase 2
                                } else {
                                    // Beide Phasen abgeschlossen
                                    repetitionIndex++;
                                    phase = 1; // Zurück zu Phase 1
                                }
                            }
                        }
                    }
                } else {
                    // Nächster Zeitplan
                    scheduleIndex++;
                    repetitionIndex = 0;
                    phase = 1;
                }
            } else {
                // Alle Zeitpläne abgeschlossen
                printf("task_water_controller.c: All watering schedules completed\n");
                ControlPump(false, PUMP_ZULAUF);
                ControlPump(false, PUMP_ABLAUF);
                configLoaded = false; // Warte auf neue Konfiguration
                finished = true;
            }
        } else {
            if (finished) {
                // Warte auf neue Konfiguration
                printf("task_water_controller.c: Waiting for new configuration...\n");
                uint32_t newConfigFlag = osEventFlagsWait(gControllerEventGroupHandle,
                		NEW_GROW_CYCLE_CONFIG_AVAILABLE,
                                                          osFlagsWaitAny, osWaitForever);

                if (newConfigFlag & NEW_GROW_CYCLE_CONFIG_AVAILABLE) {
                    // Versuche, die Konfiguration zu laden
                    if (load_grow_cycle_config(&growConfig)) {
                        printf("task_water_controller.c: Successfully loaded new GrowCycle from EEPROM\n");
                        configLoaded = true;
                        scheduleIndex = 0;
                        repetitionIndex = 0;
                        pumping = false;
                        finished = false;
                        phase = 1;
                    } else {
                        printf("task_water_controller.c: Failed to load new grow cycle config\n");
                        // Hier kannst du entscheiden, was zu tun ist
                    }

                    // Lösche das Event-Flag
                    osEventFlagsClear(gControllerEventGroupHandle, NEW_GROW_CYCLE_CONFIG_AVAILABLE);
                }
            } else {
                // Erster Ladeversuch beim Start oder nach fehlgeschlagenem Laden
                if (load_grow_cycle_config(&growConfig)) {
                    printf("task_water_controller.c: Successfully loaded GrowCycle from EEPROM\n");
                    configLoaded = true;
                    scheduleIndex = 0;
                    repetitionIndex = 0;
                    pumping = false;
                    phase = 1;
                } else {
                    printf("task_water_controller.c: No grow cycle config found. Waiting for new configuration...\n");
                    finished = true; // Warte auf neue Konfiguration
                }
            }
        }

        // Wartezeit entsprechend einstellen
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
