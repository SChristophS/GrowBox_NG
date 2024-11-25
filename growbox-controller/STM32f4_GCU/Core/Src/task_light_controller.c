/* task_light_controller.c */

#include "task_light_controller.h"
#include "state_manager.h"
#include "controller_state.h"
#include "cmsis_os.h"
#include "uart_redirect.h"
#include "hardware.h"
#include "task_hardware.h"
#include "globals.h"
#include "DS3231.h"
#include "logger.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "time_utils.h"
#include "task_light_controller.h"
#include "helper_websocket.h"

// Funktionsprototypen
static bool wait_for_start_time(struct tm *startTimeTm);
static void handle_manual_override(LightCommand *receivedCommand, bool *manualOverride, uint8_t *manualDesiredIntensity);

static void adjust_schedule_based_on_elapsed_time(
    LedSchedule *ledSchedules,
    uint8_t ledScheduleCount,
    time_t elapsedTime,
    uint8_t *scheduleIndex,
    uint8_t *repetitionIndex,
    LightState *currentState,
    uint32_t *phaseDuration,
    uint32_t *phaseStartTime,
    bool *finished);

void StartLightTask(void *argument)
{
    LOG_INFO("task_light_controller:\tStarting Light Control Task");

    // Registriere den Heartbeat
    if (!RegisterTaskHeartbeat("task_light_controller")) {
        LOG_ERROR("task_light_controller: Failed to register heartbeat");
        Error_Handler();
    }
    LOG_INFO("task_light_controller:\tHeartbeat registered successfully");

    // Initialer Lichtstatus
    ControlLight(0);
    LOG_INFO("task_light_controller:\tLight intensity set to 0 initially");

    LOG_INFO("task_light_controller:\tWaiting for 5 seconds");




    LOG_INFO("task_light_controller:\tWaiting done");

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

    // Variablen für die Zeitsteuerung
    struct tm startTimeTm;
    bool timeSynchronized = false;
    bool scheduleAdjusted = false;

    // 1. Überprüfen, ob eine Konfiguration verfügbar ist
    osMutexAcquire(gConfigAvailableMutexHandle, osWaitForever);
    bool configAvailable = gConfigAvailable;
    osMutexRelease(gConfigAvailableMutexHandle);

    if (configAvailable) {
        // Kopieren der Konfiguration aus der globalen Variable
        osMutexAcquire(gGrowCycleConfigMutexHandle, osWaitForever);
        memcpy(&growConfig, &gGrowCycleConfig, sizeof(GrowCycleConfig));
        osMutexRelease(gGrowCycleConfigMutexHandle);

        // Startzeit und Synchronisationsstatus aus globalen Variablen abrufen
        osMutexAcquire(gStartTimeMutexHandle, osWaitForever);
        memcpy(&startTimeTm, &gStartTimeTm, sizeof(struct tm));
        timeSynchronized = gTimeSynchronized;
        osMutexRelease(gStartTimeMutexHandle);

        configLoaded = true;
        LOG_INFO("task_light_controller:\tConfiguration loaded successfully at startup");
    } else {
        LOG_WARN("task_light_controller:\tNo configuration available at startup");
        configLoaded = false;
    }

    for (;;)
    {
        LOG_DEBUG("task_light_controller:\tEntering main loop iteration");
        osDelay(500);
        // Heartbeat aktualisieren
        UpdateTaskHeartbeat("task_light_controller");

        // 2. Manuelle Befehle prüfen und verarbeiten
        handle_manual_override(&receivedCommand, &manualOverride, &manualDesiredIntensity);
        if (manualOverride)
        {
            // Manuelle Übersteuerung aktiv
            LOG_INFO("task_light_controller:\tManual override activated with intensity: %d", manualDesiredIntensity);
            achieve_light_intensity(manualDesiredIntensity);
            LOG_INFO("task_light_controller:\tProcessed manual override to intensity %d", manualDesiredIntensity);
            manualOverride = false;
            //vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        // 3. Überprüfen auf neue Konfiguration
        int32_t flags = osEventFlagsWait(gControllerEventGroupHandle, NEW_GROW_CYCLE_CONFIG_AVAILABLE_LIGHT, osFlagsWaitAny, 0);
        if (flags > 0 && (flags & NEW_GROW_CYCLE_CONFIG_AVAILABLE_LIGHT))
        {
            // Neue Konfiguration ist verfügbar
            osMutexAcquire(gConfigAvailableMutexHandle, osWaitForever);
            configAvailable = gConfigAvailable;
            osMutexRelease(gConfigAvailableMutexHandle);

            if (configAvailable) {
                // Kopieren der Konfiguration aus der globalen Variable
                osMutexAcquire(gGrowCycleConfigMutexHandle, osWaitForever);
                memcpy(&growConfig, &gGrowCycleConfig, sizeof(GrowCycleConfig));
                osMutexRelease(gGrowCycleConfigMutexHandle);

                // Startzeit parsen
                if (parse_iso8601_datetime(growConfig.startGrowTime, &startTimeTm)) {
                    timeSynchronized = true;
                    LOG_INFO("task_light_controller:\tParsed startGrowTime successfully");
                } else {
                    LOG_ERROR("task_light_controller:\tFailed to parse startGrowTime");
                    timeSynchronized = false;
                }

                configLoaded = true;
                finished = false;
                scheduleIndex = 0;
                repetitionIndex = 0;
                currentState = LIGHT_OFF;
                phaseDuration = 0;
                scheduleAdjusted = false;
                LOG_INFO("task_light_controller:\tNew GrowCycleConfig loaded");
            } else {
                LOG_WARN("task_light_controller:\tConfiguration flag set but no configuration available");
                configLoaded = false;
            }
        }

        // 4. Überprüfen, ob Konfiguration geladen und nicht beendet
        if (!configLoaded || finished)
        {
            LOG_DEBUG("task_light_controller:\tNo configuration loaded or schedules are finished");
            LOG_INFO("task_light_controller:\tTurning off light as there is no active schedule");
            achieve_light_intensity(0);
            //vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        // 5. Warten bis Startzeit erreicht ist
        if (timeSynchronized && !scheduleAdjusted)
        {
            LOG_INFO("task_light_controller:\tChecking if start time has been reached");
            if (!wait_for_start_time(&startTimeTm)) {
                LOG_INFO("task_light_controller:\tCurrent time is before start time. Turning off light.");
                achieve_light_intensity(0);
                //vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }

            // 6. Zeitplan anpassen basierend auf verstrichener Zeit
            struct tm currentTimeTm;
            if (!get_current_time(&currentTimeTm)) {
                LOG_ERROR("task_light_controller:\tFailed to get current time");
                //vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }

            time_t currentSeconds = tm_to_seconds(&currentTimeTm);
            time_t startSeconds = tm_to_seconds(&startTimeTm);
            time_t elapsedTime = currentSeconds - startSeconds;

            LOG_DEBUG("task_light_controller:\tAdjusting schedule based on elapsed time: %ld seconds", elapsedTime);

            adjust_schedule_based_on_elapsed_time(
                growConfig.ledSchedules,
                growConfig.ledScheduleCount,
                elapsedTime,
                &scheduleIndex,
                &repetitionIndex,
                &currentState,
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
        LOG_DEBUG("task_light_controller:\tAutomatic mode status: %s", isAutomaticMode ? "ON" : "OFF");

        if (!isAutomaticMode)
        {
            LOG_INFO("task_light_controller:\tAutomatic mode is disabled. Turning off light.");
            achieve_light_intensity(0);
            //vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        // 8. Lichtzeitplan verarbeiten
        LOG_DEBUG("task_light_controller:\tProcessing light schedule");
        process_light_schedule(
            &growConfig,
            &scheduleIndex,
            &repetitionIndex,
            &currentState,
            &phaseStartTime,
            &phaseDuration,
            &lightIntensity,
            &finished
        );

        // Kurze Verzögerung vor der nächsten Iteration
        //vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// 2. Manuelle Befehle prüfen und verarbeiten
static void handle_manual_override(LightCommand *receivedCommand, bool *manualOverride, uint8_t *manualDesiredIntensity)
{
    if (osMessageQueueGet(xLightCommandQueueHandle, receivedCommand, NULL, 0) == osOK)
    {
        if (receivedCommand->commandType == LIGHT_COMMAND_SET_INTENSITY)
        {
            *manualOverride = true;
            *manualDesiredIntensity = receivedCommand->intensity;
            LOG_INFO("task_light_controller:\tReceived manual light command: Set intensity to %d", *manualDesiredIntensity);
        }
    }
}

// 5. Warten bis Startzeit erreicht ist
static bool wait_for_start_time(struct tm *startTimeTm)
{
    struct tm currentTimeTm;
    if (!get_current_time(&currentTimeTm)) {
        LOG_ERROR("task_light_controller:\tFailed to get current time");
        return false;
    }

    // Vergleiche aktuelle Zeit mit Startzeit
    int cmp = compare_tm(&currentTimeTm, startTimeTm);

    char currentTimeStr[20];
    char startTimeStr[20];
    strftime(currentTimeStr, sizeof(currentTimeStr), "%Y-%m-%d %H:%M:%S", &currentTimeTm);
    strftime(startTimeStr, sizeof(startTimeStr), "%Y-%m-%d %H:%M:%S", startTimeTm);

    LOG_INFO("task_light_controller:\tCurrent Time: %s", currentTimeStr);
    LOG_INFO("task_light_controller:\tStart Time:   %s", startTimeStr);
    LOG_INFO("task_light_controller:\tComparison result: %d", cmp);

    if (cmp < 0) {
        LOG_INFO("task_light_controller:\tCurrent time is before start time. Waiting...");
        return false;
    }

    return true;
}






// 6. Zeitplan anpassen basierend auf verstrichener Zeit
static void adjust_schedule_based_on_elapsed_time(
    LedSchedule *ledSchedules,
    uint8_t ledScheduleCount,
    time_t elapsedTime,
    uint8_t *scheduleIndex,
    uint8_t *repetitionIndex,
    LightState *currentState,
    uint32_t *phaseDuration,
    uint32_t *phaseStartTime,
    bool *finished
) {
    if (ledSchedules == NULL || scheduleIndex == NULL || repetitionIndex == NULL ||
        currentState == NULL || phaseDuration == NULL || phaseStartTime == NULL || finished == NULL) {
        LOG_ERROR("task_light_controller:\tInvalid arguments to adjust_schedule_based_on_elapsed_time");
        return;
    }

    time_t accumulatedTime = 0;

    LOG_DEBUG("task_light_controller:\telapsedTime: %ld seconds", elapsedTime);

    // Durchlaufen aller Zeitpläne
    for (uint8_t i = 0; i < ledScheduleCount; i++) {
        LedSchedule *schedule = &ledSchedules[i];

        // Validierung der Schedule-Daten
        if (schedule->durationOn == 0 && schedule->durationOff == 0) {
            LOG_WARN("task_light_controller:\tSchedule %d has zero duration for both On and Off phases. Skipping.", i);
            continue;
        }

        // Durchlaufen aller Wiederholungen innerhalb eines Zeitplans
        for (uint8_t rep = 0; rep < schedule->repetition; rep++) {
            // Dauer der EIN- und AUS-Phasen in Sekunden
            time_t onDuration = schedule->durationOn;
            time_t offDuration = schedule->durationOff;

            // Gesamtdauer dieser Wiederholung
            time_t repetitionDuration = onDuration + offDuration;

            if (repetitionDuration == 0) {
                LOG_WARN("task_light_controller:\tRepetition %d of Schedule %d has zero duration. Skipping.", rep, i);
                continue;
            }

            LOG_DEBUG("task_light_controller:\tChecking Schedule %d, Repetition %d", i, rep);
            LOG_DEBUG("task_light_controller:\tAccumulated Time: %ld seconds", accumulatedTime);
            LOG_DEBUG("task_light_controller:\tRepetition Duration: %ld seconds", repetitionDuration);

            if (elapsedTime < accumulatedTime + repetitionDuration) {
                // Wir befinden uns innerhalb dieser Wiederholung
                *scheduleIndex = i;
                *repetitionIndex = rep;

                time_t timeIntoRepetition = elapsedTime - accumulatedTime;
                if (timeIntoRepetition < onDuration) {
                    // Wir sind in der EIN-Phase
                    *currentState = LIGHT_ON;
                    *phaseDuration = (onDuration - timeIntoRepetition) * 1000; // in Millisekunden
                } else {
                    // Wir sind in der AUS-Phase
                    *currentState = LIGHT_OFF;
                    *phaseDuration = (repetitionDuration - timeIntoRepetition) * 1000; // in Millisekunden
                }
                *phaseStartTime = osKernelGetTickCount();
                *finished = false;
                LOG_INFO("task_light_controller:\tAdjusted schedule to Schedule %d, Repetition %d, State %s, Phase Duration %lu ms",
                         *scheduleIndex, *repetitionIndex, (*currentState == LIGHT_ON) ? "ON" : "OFF", *phaseDuration);
                return;
            }

            accumulatedTime += repetitionDuration;
        }
    }

    // Wenn die verstrichene Zeit alle Zeitpläne übersteigt
    *scheduleIndex = ledScheduleCount;
    *finished = true;
    LOG_INFO("task_light_controller:\tAll schedules are finished based on elapsed time");
}




void adjust_schedule_for_elapsed_time(LedSchedule *ledSchedules, uint8_t ledScheduleCount, time_t elapsedTime, uint8_t *scheduleIndex, uint8_t *repetitionIndex, LightState *currentState, uint32_t *phaseDuration, uint32_t *phaseStartTime, bool *finished) {
    time_t accumulatedTime = 0;

    for (uint8_t i = 0; i < ledScheduleCount; i++) {
        LedSchedule *schedule = &ledSchedules[i];

        // Für jede Wiederholung
        for (uint8_t rep = 0; rep < schedule->repetition; rep++) {
            // Dauer der Ein- und Aus-Phase
            time_t onDuration = schedule->durationOn;
            time_t offDuration = schedule->durationOff;

            // Gesamtdauer dieser Wiederholung
            time_t repetitionDuration = onDuration + offDuration;

            if (elapsedTime < accumulatedTime + repetitionDuration) {
                // Wir befinden uns innerhalb dieser Wiederholung
                *scheduleIndex = i;
                *repetitionIndex = rep;

                time_t timeIntoRepetition = elapsedTime - accumulatedTime;
                if (timeIntoRepetition < onDuration) {
                    // Wir sind in der EIN-Phase
                    *currentState = LIGHT_ON;
                    *phaseDuration = (onDuration - timeIntoRepetition) * 1000; // in Millisekunden
                } else {
                    // Wir sind in der AUS-Phase
                    *currentState = LIGHT_OFF;
                    *phaseDuration = (repetitionDuration - timeIntoRepetition) * 1000; // in Millisekunden
                }
                *phaseStartTime = osKernelGetTickCount();
                *finished = false;
                LOG_INFO("task_light_controller:\tAdjusted schedule to Schedule %d, Repetition %d, State %s, Phase Duration %lu ms", *scheduleIndex, *repetitionIndex, (*currentState == LIGHT_ON) ? "ON" : "OFF", *phaseDuration);
                return;
            }

            accumulatedTime += repetitionDuration;
        }
    }

    // Wenn die verstrichene Zeit alle Zeitpläne übersteigt
    *scheduleIndex = ledScheduleCount;
    *finished = true;
    LOG_INFO("task_light_controller:\tAll schedules are finished based on elapsed time");
}



// 8. Lichtzeitplan verarbeiten
void process_light_schedule(
    GrowCycleConfig *growConfig,
    uint8_t *scheduleIndex,
    uint8_t *repetitionIndex,
    LightState *currentState,
    uint32_t *phaseStartTime,
    uint32_t *phaseDuration,
    uint8_t *lightIntensity,
    bool *finished
) {
    if (growConfig == NULL || scheduleIndex == NULL || repetitionIndex == NULL ||
        currentState == NULL || phaseStartTime == NULL || phaseDuration == NULL ||
        lightIntensity == NULL || finished == NULL) {
        LOG_ERROR("task_light_controller:\tInvalid arguments to process_light_schedule");
        return;
    }

    if (*scheduleIndex < growConfig->ledScheduleCount) {
        LedSchedule *schedule = &growConfig->ledSchedules[*scheduleIndex];

        if (*repetitionIndex < schedule->repetition) {
            // Starten der Phase, falls nicht bereits laufend
            if (*phaseDuration == 0) {
                if (*currentState == LIGHT_OFF) {
                    // Licht einschalten
                    LOG_INFO("task_light_controller:\tTurning light ON (Schedule %d, Repetition %d)", *scheduleIndex, *repetitionIndex);
                    *lightIntensity = 0;
                    achieve_light_intensity(*lightIntensity);
                    *currentState = LIGHT_ON;
                    *phaseDuration = schedule->durationOn * 1000; // in Millisekunden
                    LOG_DEBUG("task_light_controller:\tStarted LIGHT_ON phase for %lu ms", *phaseDuration);
                } else {
                    // Licht ausschalten
                    LOG_INFO("task_light_controller:\tTurning light OFF (Schedule %d, Repetition %d)", *scheduleIndex, *repetitionIndex);
                    *lightIntensity = 100;
                    achieve_light_intensity(*lightIntensity);
                    *currentState = LIGHT_OFF;
                    *phaseDuration = schedule->durationOff * 1000; // in Millisekunden
                    (*repetitionIndex)++;
                    LOG_DEBUG("task_light_controller:\tStarted LIGHT_OFF phase for %lu ms", *phaseDuration);
                }
                *phaseStartTime = osKernelGetTickCount();
            } else {
                // Überprüfe, ob die Phase abgeschlossen ist
                uint32_t elapsedTimeMs = osKernelGetTickCount() - *phaseStartTime;
                LOG_DEBUG("task_light_controller:\tPhase elapsed time: %lu ms / %lu ms", elapsedTimeMs, *phaseDuration);
                if (elapsedTimeMs >= *phaseDuration) {
                    // Beende die aktuelle Phase
                    LOG_INFO("task_light_controller:\tPhase duration completed. Preparing for next phase.");
                    *phaseDuration = 0; // Bereit für die nächste Phase
                }
            }
        } else {
            // Nächster Zeitplan
            LOG_INFO("task_light_controller:\tCompleted all repetitions for Schedule %d. Moving to next schedule.", *scheduleIndex);
            (*scheduleIndex)++;
            *repetitionIndex = 0;
            *currentState = LIGHT_OFF;
            *phaseDuration = 0;
            LOG_INFO("task_light_controller:\tMoving to next LED schedule (Index %d)", *scheduleIndex);
        }
    } else {
        // Alle Zeitpläne abgeschlossen
        LOG_INFO("task_light_controller:\tAll LED schedules completed");
        *finished = true;

        // Licht sicherheitshalber ausschalten
        LOG_INFO("task_light_controller:\tEnsuring light is turned off.");
        *lightIntensity = 0;
        achieve_light_intensity(*lightIntensity);
    }
}


void UpdateLightControllerState(uint8_t lightIntensity) {

	osMutexAcquire(gControllerStateMutexHandle, osWaitForever);


	gControllerState.lightIntensity = lightIntensity;
	LOG_DEBUG("task_light_controller:\tUpdated lightIntensity in gControllerState to %d", lightIntensity);

	osEventFlagsSet(gControllerEventGroupHandle, LIGHT_INTENSITY_CHANGED_BIT);

	// send status update
	send_status_update(MESSAGE_TYPE_STATUS_UPDATE, DEVICE_CONTROLLER, TARGET_LIGHT_INTENSITY, lightIntensity);


    osMutexRelease(gControllerStateMutexHandle);


}


void ControlLight(uint8_t lightIntensity) {
    LOG_DEBUG("task_light_controller:\tSending new PWM value to Hardware Task");

    // Erstelle Hardwarebefehl
    HardwareCommand cmd;
    cmd.commandType = COMMAND_CONTROL_LIGHT;
    cmd.intensity = lightIntensity;

    // Sende Befehl an Hardware-Task
    osStatus_t status = osMessageQueuePut(xHardwareQueueHandle, &cmd, 0, 0);
    if (status != osOK) {
        LOG_ERROR("task_light_controller:\tFailed to put HardwareCommand in queue. Status: %d", status);
    }
}


void achieve_light_intensity(uint8_t intensity)
{
    LOG_DEBUG("task_light_controller:\tSetting light intensity to %d", intensity);

    if (gControllerState.lightIntensity != intensity) {
        UpdateLightControllerState(intensity);
        ControlLight(intensity);
        LOG_INFO("task_light_controller:\tLight intensity changed to %d", intensity);
    }
    else {
        LOG_DEBUG("task_light_controller:\tNo change needed. Current intensity: %d, Desired intensity: %d",
                  gControllerState.lightIntensity, intensity);
    }
}



time_t get_current_timestamp(void)
{
    struct tm currentTimeTm;
    if (!get_current_time(&currentTimeTm)) {
        LOG_ERROR("task_light_controller:\tFailed to get current time");
        return -1;
    }

    return mktime(&currentTimeTm);
}
