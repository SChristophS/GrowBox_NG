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

// Funktionsprototypen
void UpdateLightControllerState(uint8_t lightIntensity);
void ControlLight(uint8_t lightIntensity);
void achieve_light_intensity(uint8_t intensity);
bool parse_iso8601_datetime(const char *datetime_str, struct tm *tm_time);
time_t get_current_timestamp(void);
static bool wait_for_start_time(struct tm *startTimeTm);

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

void adjust_schedule_for_elapsed_time(
    LedSchedule *ledSchedules,
    uint8_t ledScheduleCount,
    time_t elapsedTime,
    uint8_t *scheduleIndex,
    uint8_t *repetitionIndex,
    LightState *currentState,
    uint32_t *phaseDuration,
    uint32_t *phaseStartTime,
    bool *finished);

// Neue Hilfsfunktionen
static void handle_manual_override(LightCommand *receivedCommand, bool *manualOverride, uint8_t *manualDesiredIntensity);

void process_light_schedule(
    GrowCycleConfig *growConfig,
    uint8_t *scheduleIndex,
    uint8_t *repetitionIndex,
    LightState *currentState,
    uint32_t *phaseStartTime,
    uint32_t *phaseDuration,
    uint8_t *lightIntensity,
    bool *finished
);

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

    // Variablen für die Zeitsteuerung
    struct tm startTimeTm;
    bool timeSynchronized = false;
    bool scheduleAdjusted = false;

    // 1. Konfiguration laden und Startzeit parsen
    configLoaded = load_grow_cycle_config(&growConfig, &startTimeTm, &timeSynchronized);

    for (;;)
    {
        // 2. Manuelle Befehle prüfen und verarbeiten
        handle_manual_override(&receivedCommand, &manualOverride, &manualDesiredIntensity);
        if (manualOverride)
        {
            // Manuelle Übersteuerung aktiv
            achieve_light_intensity(manualDesiredIntensity);
            manualOverride = false;
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        // 3. Überprüfen auf neue Konfiguration
        int32_t flags = osEventFlagsWait(gControllerEventGroupHandle, NEW_GROW_CYCLE_CONFIG_AVAILABLE_LIGHT, osFlagsWaitAny, 0);
        if (flags > 0 && (flags & NEW_GROW_CYCLE_CONFIG_AVAILABLE_LIGHT))
        {
            // Neue Konfiguration laden
        	configLoaded = load_grow_cycle_config(&growConfig, &startTimeTm, &timeSynchronized);
            finished = false;
            scheduleIndex = 0;
            repetitionIndex = 0;
            currentState = LIGHT_OFF;
            phaseDuration = 0;
            scheduleAdjusted = false;
            LOG_INFO("task_light_controller:\tNew GrowCycleConfig loaded");
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        // 4. Überprüfen, ob Konfiguration geladen und nicht beendet
        if (!configLoaded || finished)
        {
            LOG_DEBUG("task_light_controller:\tNo configuration loaded or schedules are finished");
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
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        // 8. Lichtzeitplan verarbeiten
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
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* Implementierung der Hilfsfunktionen */



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
        LOG_ERROR("task_light_controller: Failed to get current time");
        return false;
    }

    // Vergleiche aktuelle Zeit mit Startzeit
    int cmp = compare_tm(&currentTimeTm, startTimeTm);

    char currentTimeStr[20];
    char startTimeStr[20];
    strftime(currentTimeStr, sizeof(currentTimeStr), "%Y-%m-%d %H:%M:%S", &currentTimeTm);
    strftime(startTimeStr, sizeof(startTimeStr), "%Y-%m-%d %H:%M:%S", startTimeTm);

    LOG_INFO("task_light_controller:\tcurrentTime: %s", currentTimeStr);
    LOG_INFO("task_light_controller:\tstartTime:   %s", startTimeStr);
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
    time_t accumulatedTime = 0;

    LOG_DEBUG("task_light_controller:\telapsedTime: %ld seconds", elapsedTime);

    // Durchlaufen aller Zeitpläne
    for (uint8_t i = 0; i < ledScheduleCount; i++) {
        LedSchedule *schedule = &ledSchedules[i];

        // Durchlaufen aller Wiederholungen innerhalb eines Zeitplans
        for (uint8_t rep = 0; rep < schedule->repetition; rep++) {
            // Dauer der EIN- und AUS-Phasen in Sekunden
            time_t onDuration = schedule->durationOn;
            time_t offDuration = schedule->durationOff;

            // Gesamtdauer dieser Wiederholung
            time_t repetitionDuration = onDuration + offDuration;

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
    if (*scheduleIndex < growConfig->ledScheduleCount) {
        LedSchedule *schedule = &growConfig->ledSchedules[*scheduleIndex];

        if (*repetitionIndex < schedule->repetition) {
            // Starten der Phase, falls nicht bereits laufend
            if (*phaseDuration == 0) {
                if (*currentState == LIGHT_OFF) {
                    // Licht einschalten
                    LOG_INFO("task_light_controller:\tTurning light ON (Schedule %d, Repetition %d)", *scheduleIndex, *repetitionIndex);
                    *lightIntensity = 100; // Maximale Intensität
                    achieve_light_intensity(*lightIntensity);
                    *currentState = LIGHT_ON;
                    *phaseDuration = schedule->durationOn * 1000; // in Millisekunden
                } else {
                    // Licht ausschalten
                    LOG_INFO("task_light_controller:\tTurning light OFF (Schedule %d, Repetition %d)", *scheduleIndex, *repetitionIndex);
                    *lightIntensity = 0;
                    achieve_light_intensity(*lightIntensity);
                    *currentState = LIGHT_OFF;
                    *phaseDuration = schedule->durationOff * 1000; // in Millisekunden
                    (*repetitionIndex)++;
                }
                *phaseStartTime = osKernelGetTickCount();
            } else {
                // Überprüfe, ob die Phase abgeschlossen ist
                uint32_t elapsedTimeMs = osKernelGetTickCount() - *phaseStartTime;
                if (elapsedTimeMs >= *phaseDuration) {
                    // Beende die aktuelle Phase
                    *phaseDuration = 0; // Bereit für die nächste Phase
                }
            }
        } else {
            // Nächster Zeitplan
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
        *lightIntensity = 0;
        achieve_light_intensity(*lightIntensity);
    }
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

void achieve_light_intensity(uint8_t intensity)
{
    LOG_DEBUG("task_light_controller:\tSetting light intensity to %d", intensity);
    UpdateLightControllerState(intensity);
    ControlLight(intensity);
// Restliche Funktionen (parse_iso8601_datetime, get_current_time, etc.) bleiben wie zuvor oder können ebenfalls vereinfacht werden.
};


time_t get_current_timestamp(void)
{
    struct tm currentTimeTm;
    if (!get_current_time(&currentTimeTm)) {
        LOG_ERROR("task_light_controller:\tFailed to get current time");
        return -1;
    }

    return mktime(&currentTimeTm);
}
