#include "task_water_controller.h"
#include "state_manager.h"
#include "controller_state.h"
#include "cmsis_os.h"
#include "uart_redirect.h"
#include "hardware.h"
#include "task_hardware.h"
#include "globals.h"
#include "logger.h"
#include "time_utils.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Funktionsprototypen
void UpdatePumpState(bool enable, uint8_t pumpId);
void ControlPump(bool enable, uint8_t pumpId);
void achieve_water_state_full(void);
void achieve_water_state_empty(void);
static void handle_manual_override(WaterCommand *receivedCommand, bool *manualOverride, WaterState *manualDesiredState);
static bool wait_for_start_time(struct tm *startTimeTm);
void adjust_schedule_based_on_elapsed_time(
    WateringSchedule *wateringSchedules,
    uint8_t wateringScheduleCount,
    time_t elapsedTime,
    uint8_t *scheduleIndex,
    uint8_t *repetitionIndex,
    WateringPhase *currentPhase,
    uint32_t *phaseDuration,
    uint32_t *phaseStartTime,
    bool *finished);
void process_watering_schedule(
    GrowCycleConfig *growConfig,
    uint8_t *scheduleIndex,
    uint8_t *repetitionIndex,
    WateringPhase *currentPhase,
    uint32_t *phaseStartTime,
    uint32_t *phaseDuration,
    bool *finished,
    bool *isPumping);

// Konstanten
#define MAX_FILL_DURATION_MS 300000  // 5 Minuten
#define MAX_EMPTY_DURATION_MS 300000 // 5 Minuten

void StartWaterControllerTask(void *argument)
{
    LOG_INFO("task_water_controller:\tStarting Water Controller Task");

    // Variablen für die Wassersteuerung
    GrowCycleConfig growConfig;
    bool configLoaded = false;
    bool finished = false;
    WateringPhase currentPhase = PHASE_FULL;
    uint32_t phaseStartTime = 0;
    uint32_t phaseDuration = 0;
    bool isPumping = false;

    uint8_t scheduleIndex = 0;
    uint8_t repetitionIndex = 0;

    // Variablen für manuelle Befehle
    WaterCommand receivedCommand;
    bool manualOverride = false;
    WaterState manualDesiredState;

    // Variablen für die Zeitsteuerung
    struct tm startTimeTm;
    bool timeSynchronized = false;
    bool scheduleAdjusted = false;

    // 1. Konfiguration laden und Startzeit parsen
    configLoaded = load_grow_cycle_config(&growConfig, &startTimeTm, &timeSynchronized);

    for (;;)
    {
        // 2. Manuelle Befehle prüfen und verarbeiten
        handle_manual_override(&receivedCommand, &manualOverride, &manualDesiredState);
        if (manualOverride)
        {
            // Manuelle Übersteuerung aktiv
            if (manualDesiredState == WATER_STATE_FULL) {
                achieve_water_state_full();
            } else if (manualDesiredState == WATER_STATE_EMPTY) {
                achieve_water_state_empty();
            }
            manualOverride = false;
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        // 3. Überprüfen auf neue Konfiguration
        int32_t flags = osEventFlagsWait(gControllerEventGroupHandle, NEW_GROW_CYCLE_CONFIG_AVAILABLE_WATER, osFlagsWaitAny, 0);
        if (flags > 0 && (flags & NEW_GROW_CYCLE_CONFIG_AVAILABLE_WATER))
        {
            // Neue Konfiguration laden
            configLoaded = load_grow_cycle_config(&growConfig, &startTimeTm, &timeSynchronized);
            finished = false;
            scheduleIndex = 0;
            repetitionIndex = 0;
            currentPhase = PHASE_FULL;
            phaseDuration = 0;
            scheduleAdjusted = false;
            LOG_INFO("task_water_controller:\tNew GrowCycleConfig loaded");
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        // 4. Überprüfen, ob Konfiguration geladen und nicht beendet
        if (!configLoaded || finished)
        {
            LOG_DEBUG("task_water_controller:\tNo configuration loaded or schedules are finished");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        // 5. Warten bis Startzeit erreicht ist
        if (timeSynchronized && !scheduleAdjusted)
        {
            if (!wait_for_start_time(&startTimeTm)) {
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }

            // 6. Zeitplan anpassen basierend auf verstrichener Zeit
            struct tm currentTimeTm;
            if (!get_current_time(&currentTimeTm)) {
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }

            time_t currentSeconds = tm_to_seconds(&currentTimeTm);
            time_t startSeconds = tm_to_seconds(&startTimeTm);
            time_t elapsedTime = currentSeconds - startSeconds;

            adjust_schedule_based_on_elapsed_time(
                growConfig.wateringSchedules,
                growConfig.wateringScheduleCount,
                elapsedTime,
                &scheduleIndex,
                &repetitionIndex,
                &currentPhase,
                &phaseDuration,
                &phaseStartTime,
                &finished
            );
            scheduleAdjusted = true;
        }

        // 7. Automatikmodus prüfen
        osMutexAcquire(gAutomaticModeHandle, osWaitForever);
        bool isAutomaticMode = automaticMode;
        osMutexRelease(gAutomaticModeHandle);
        LOG_DEBUG("task_water_controller:\tAutomatic mode status: %s", isAutomaticMode ? "ON" : "OFF");

        if (!isAutomaticMode)
        {
            LOG_INFO("task_water_controller:\tAutomatic mode is disabled. Turning pumps off.");
            ControlPump(false, PUMP_ZULAUF);
            ControlPump(false, PUMP_ABLAUF);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        // 8. Wasserzeitplan verarbeiten
        process_watering_schedule(
            &growConfig,
            &scheduleIndex,
            &repetitionIndex,
            &currentPhase,
            &phaseStartTime,
            &phaseDuration,
            &finished,
            &isPumping
        );

        // Kurze Verzögerung vor der nächsten Iteration
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* Implementierung der Hilfsfunktionen */

// 2. Manuelle Befehle prüfen und verarbeiten
static void handle_manual_override(WaterCommand *receivedCommand, bool *manualOverride, WaterState *manualDesiredState)
{
    if (osMessageQueueGet(xWaterCommandQueueHandle, receivedCommand, NULL, 0) == osOK)
    {
        if (receivedCommand->commandType == WATER_COMMAND_SET_STATE)
        {
            *manualOverride = true;
            *manualDesiredState = receivedCommand->desiredState;
            LOG_INFO("task_water_controller:\tReceived manual water command: %s",
                     (*manualDesiredState == WATER_STATE_FULL) ? "FULL" : "EMPTY");
        }
    }
}

// 5. Warten bis Startzeit erreicht ist
static bool wait_for_start_time(struct tm *startTimeTm)
{
    struct tm currentTimeTm;
    if (!get_current_time(&currentTimeTm)) {
        LOG_ERROR("task_water_controller: Failed to get current time");
        return false;
    }

    // Vergleiche aktuelle Zeit mit Startzeit
    int cmp = compare_tm(&currentTimeTm, startTimeTm);

    char currentTimeStr[20];
    char startTimeStr[20];
    strftime(currentTimeStr, sizeof(currentTimeStr), "%Y-%m-%d %H:%M:%S", &currentTimeTm);
    strftime(startTimeStr, sizeof(startTimeStr), "%Y-%m-%d %H:%M:%S", startTimeTm);

    LOG_INFO("task_water_controller:\tcurrentTime: %s", currentTimeStr);
    LOG_INFO("task_water_controller:\tstartTime:   %s", startTimeStr);
    LOG_INFO("task_water_controller:\tComparison result: %d", cmp);

    if (cmp < 0) {
        LOG_INFO("task_water_controller:\tCurrent time is before start time. Waiting...");
        return false;
    }

    return true;
}

// 6. Zeitplan anpassen basierend auf verstrichener Zeit
void adjust_schedule_based_on_elapsed_time(
    WateringSchedule *wateringSchedules,
    uint8_t wateringScheduleCount,
    time_t elapsedTime,
    uint8_t *scheduleIndex,
    uint8_t *repetitionIndex,
    WateringPhase *currentPhase,
    uint32_t *phaseDuration,
    uint32_t *phaseStartTime,
    bool *finished)
{
    time_t accumulatedTime = 0;

    LOG_DEBUG("task_water_controller:\telapsedTime: %ld seconds", elapsedTime);

    // Durchlaufen aller Zeitpläne
    for (uint8_t i = 0; i < wateringScheduleCount; i++) {
        WateringSchedule *schedule = &wateringSchedules[i];

        // Durchlaufen aller Wiederholungen innerhalb eines Zeitplans
        for (uint8_t rep = 0; rep < schedule->repetition; rep++) {
            // Dauer der FULL- und EMPTY-Phasen in Sekunden
            time_t fullDuration = schedule->duration_full;
            time_t emptyDuration = schedule->duration_empty;

            // Gesamtdauer dieser Wiederholung
            time_t repetitionDuration = fullDuration + emptyDuration;

            LOG_DEBUG("task_water_controller:\tChecking Schedule %d, Repetition %d", i, rep);
            LOG_DEBUG("task_water_controller:\tAccumulated Time: %ld seconds", accumulatedTime);
            LOG_DEBUG("task_water_controller:\tRepetition Duration: %ld seconds", repetitionDuration);

            if (elapsedTime < accumulatedTime + repetitionDuration) {
                // Wir befinden uns innerhalb dieser Wiederholung
                *scheduleIndex = i;
                *repetitionIndex = rep;

                time_t timeIntoRepetition = elapsedTime - accumulatedTime;
                if (timeIntoRepetition < fullDuration) {
                    // Wir sind in der FULL-Phase
                    *currentPhase = PHASE_FULL;
                    *phaseDuration = (fullDuration - timeIntoRepetition) * 1000; // in Millisekunden
                } else {
                    // Wir sind in der EMPTY-Phase
                    *currentPhase = PHASE_EMPTY;
                    *phaseDuration = (repetitionDuration - timeIntoRepetition) * 1000; // in Millisekunden
                }
                *phaseStartTime = osKernelGetTickCount();
                *finished = false;
                LOG_INFO("task_water_controller:\tAdjusted schedule to Schedule %d, Repetition %d, Phase %s, Phase Duration %lu ms",
                         *scheduleIndex, *repetitionIndex, (*currentPhase == PHASE_FULL) ? "FULL" : "EMPTY", *phaseDuration);
                return;
            }

            accumulatedTime += repetitionDuration;
        }
    }

    // Wenn die verstrichene Zeit alle Zeitpläne übersteigt
    *scheduleIndex = wateringScheduleCount;
    *finished = true;
    LOG_INFO("task_water_controller:\tAll schedules are finished based on elapsed time");
}

// 8. Wasserzeitplan verarbeiten
void process_watering_schedule(
    GrowCycleConfig *growConfig,
    uint8_t *scheduleIndex,
    uint8_t *repetitionIndex,
    WateringPhase *currentPhase,
    uint32_t *phaseStartTime,
    uint32_t *phaseDuration,
    bool *finished,
    bool *isPumping)
{
    if (*scheduleIndex < growConfig->wateringScheduleCount) {
        WateringSchedule *schedule = &growConfig->wateringSchedules[*scheduleIndex];

        if (*repetitionIndex < schedule->repetition) {
            // Starten der Phase, falls nicht bereits laufend
            if (*phaseDuration == 0) {
                if (*currentPhase == PHASE_FULL) {
                    // Start der Füllphase
                    LOG_INFO("task_water_controller:\tStarting PHASE_FULL (Schedule %d, Repetition %d)", *scheduleIndex, *repetitionIndex);
                    achieve_water_state_full();
                    *phaseDuration = schedule->duration_full * 1000; // in Millisekunden
                } else {
                    // Start der Entleerungsphase
                    LOG_INFO("task_water_controller:\tStarting PHASE_EMPTY (Schedule %d, Repetition %d)", *scheduleIndex, *repetitionIndex);
                    achieve_water_state_empty();
                    *phaseDuration = schedule->duration_empty * 1000; // in Millisekunden
                }
                *phaseStartTime = osKernelGetTickCount();
            } else {
                // Überprüfe, ob die Phase abgeschlossen ist
                uint32_t elapsedTimeMs = osKernelGetTickCount() - *phaseStartTime;
                if (elapsedTimeMs >= *phaseDuration) {
                    // Beende die aktuelle Phase
                    *phaseDuration = 0; // Bereit für die nächste Phase
                    // Wechsel zur nächsten Phase
                    if (*currentPhase == PHASE_FULL) {
                        *currentPhase = PHASE_EMPTY;
                    } else {
                        *currentPhase = PHASE_FULL;
                        (*repetitionIndex)++;
                    }
                }
            }
        } else {
            // Nächster Zeitplan
            (*scheduleIndex)++;
            *repetitionIndex = 0;
            *currentPhase = PHASE_FULL;
            *phaseDuration = 0;
            LOG_INFO("task_water_controller:\tMoving to next watering schedule (Index %d)", *scheduleIndex);
        }
    } else {
        // Alle Zeitpläne abgeschlossen
        LOG_INFO("task_water_controller:\tAll watering schedules completed");
        *finished = true;

        // Pumpen sicherheitshalber ausschalten
        ControlPump(false, PUMP_ZULAUF);
        ControlPump(false, PUMP_ABLAUF);
    }
}

void UpdatePumpState(bool enable, uint8_t pumpId)
{
    osMutexAcquire(gControllerStateMutexHandle, osWaitForever);

    if (pumpId == PUMP_ZULAUF) {
        if (gControllerState.pumpeZulauf != enable) {
            gControllerState.pumpeZulauf = enable;
            LOG_DEBUG("task_water_controller:\tUpdated pumpeZulauf to %s", enable ? "ON" : "OFF");
        }
    } else if (pumpId == PUMP_ABLAUF) {
        if (gControllerState.pumpeAblauf != enable) {
            gControllerState.pumpeAblauf = enable;
            LOG_DEBUG("task_water_controller:\tUpdated pumpeAblauf to %s", enable ? "ON" : "OFF");
        }
    }

    osMutexRelease(gControllerStateMutexHandle);
}

void ControlPump(bool enable, uint8_t pumpId)
{
    LOG_DEBUG("task_water_controller:\tControlPump - PumpID %d, Enable: %s", pumpId, enable ? "true" : "false");

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
    LOG_INFO("task_water_controller:\tStarting process to achieve state 'FULL'");

    // Deaktivieren der Pumpen vor Beginn
    ControlPump(false, PUMP_ZULAUF);
    ControlPump(false, PUMP_ABLAUF);

    // Starte die Pumpe, um das Becken zu füllen
    ControlPump(true, PUMP_ZULAUF);

    // Warte, bis der obere Sensor aktiviert ist oder ein Timeout eintritt
    uint32_t startTime = osKernelGetTickCount();
    uint32_t timeout = MAX_FILL_DURATION_MS; // Timeout-Dauer

    while (1) {
        osMutexAcquire(gControllerStateMutexHandle, osWaitForever);
        bool sensorOben = gControllerState.sensorOben;
        osMutexRelease(gControllerStateMutexHandle);

        if (sensorOben) {
            LOG_INFO("task_water_controller:\tState 'FULL' achieved (sensorOben is true)");
            break;
        }

        if ((osKernelGetTickCount() - startTime) > timeout) {
            LOG_WARN("task_water_controller:\tTimeout while trying to achieve state 'FULL'");
            break;
        }

        osDelay(pdMS_TO_TICKS(100));
    }

    // Pumpe ausschalten
    ControlPump(false, PUMP_ZULAUF);
    LOG_INFO("task_water_controller:\tPump turned off after achieving state 'FULL'");
}

void achieve_water_state_empty(void)
{
    LOG_INFO("task_water_controller:\tStarting process to achieve state 'EMPTY'");

    // Deaktivieren der Pumpen vor Beginn
    ControlPump(false, PUMP_ZULAUF);
    ControlPump(false, PUMP_ABLAUF);

    // Starte die Pumpe, um das Becken zu entleeren
    ControlPump(true, PUMP_ABLAUF);

    // Warte, bis beide Sensoren deaktiviert sind oder ein Timeout eintritt
    uint32_t startTime = osKernelGetTickCount();
    uint32_t timeout = MAX_EMPTY_DURATION_MS; // Timeout-Dauer

    while (1) {
        osMutexAcquire(gControllerStateMutexHandle, osWaitForever);
        bool sensorOben = gControllerState.sensorOben;
        bool sensorUnten = gControllerState.sensorUnten;
        osMutexRelease(gControllerStateMutexHandle);

        if (!sensorOben && !sensorUnten) {
            LOG_INFO("task_water_controller:\tState 'EMPTY' achieved (both sensors are false)");
            break;
        }

        if ((osKernelGetTickCount() - startTime) > timeout) {
            LOG_WARN("task_water_controller:\tTimeout while trying to achieve state 'EMPTY'");
            break;
        }

        osDelay(pdMS_TO_TICKS(100));
    }

    // Pumpe ausschalten
    ControlPump(false, PUMP_ABLAUF);
    LOG_INFO("task_water_controller:\tPump turned off after achieving state 'EMPTY'");
}
