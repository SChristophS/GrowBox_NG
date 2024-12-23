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
#include "helper_websocket.h"

// Function Prototypes
static bool wait_for_start_time(struct tm *startTimeTm);
static void handle_manual_override(void);
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
    LOG_INFO("task_light_controller: waiting for start");
    osDelay(10000);
    LOG_INFO("task_light_controller: started");

    // register heartbeat
    if (!RegisterTaskHeartbeat("task_light_controller")) {
        LOG_ERROR("task_light_controller: Fehler beim Registrieren des Heartbeats");
        Error_Handler();
    } else {
    	LOG_INFO("task_light_controller: Heartbeat erfolgreich registriert");
    };

    // workaround to set lights to 0
    // @todo: Fix workaroung
    achieve_light_intensity(1);
    achieve_light_intensity(0);

    // necesarry variables
    GrowCycleConfig growConfig;
    bool configLoaded = false;
    bool finished = false;

    LightState currentState = LIGHT_OFF;
    uint32_t phaseStartTime = 0;
    uint32_t phaseDuration = 0;

    uint8_t scheduleIndex = 0;
    uint8_t repetitionIndex = 0;
    uint8_t lightIntensity = 0;

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
        LOG_INFO("task_light_controller: Konfiguration beim Start erfolgreich geladen");
    } else {
        LOG_WARN("task_light_controller: Keine Konfiguration beim Start verfügbar");
        configLoaded = false;
    }

    for (;;)
    {
        osDelay(100);
        // Heartbeat aktualisieren
        UpdateTaskHeartbeat("task_light_controller");

        // 2. Manuelle Befehle prüfen und verarbeiten
        handle_manual_override();

        if (manualMode)
        {
            // Manuelle Übersteuerung aktiv
            LOG_INFO("task_light_controller: manual control active!");
            // Weiter zur nächsten Iteration, ohne manualOverride zurückzusetzen
            continue;
        }

        // 3. Überprüfen auf neue Konfiguration
        int32_t flags = osEventFlagsWait(gControllerEventGroupHandle, NEW_GROW_CYCLE_CONFIG_AVAILABLE_LIGHT, osFlagsWaitAny, 0);
        if ((flags & NEW_GROW_CYCLE_CONFIG_AVAILABLE_LIGHT) != 0)
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
                    LOG_INFO("task_light_controller: startGrowTime erfolgreich geparst");
                } else {
                    LOG_ERROR("task_light_controller: Fehler beim Parsen von startGrowTime");
                    timeSynchronized = false;
                }

                configLoaded = true;
                finished = false;
                scheduleIndex = 0;
                repetitionIndex = 0;
                currentState = LIGHT_OFF;
                phaseDuration = 0;
                scheduleAdjusted = false;
                LOG_INFO("task_light_controller: Neue GrowCycleConfig geladen");
            } else {
                LOG_WARN("task_light_controller: Konfigurationsflag gesetzt, aber keine Konfiguration verfügbar");
                configLoaded = false;
            }
        }

        // 4. Überprüfen, ob Konfiguration geladen und nicht beendet
        if (!configLoaded || finished)
        {
            LOG_DEBUG("task_light_controller: Keine Konfiguration geladen oder Zeitpläne sind abgeschlossen");
            LOG_INFO("task_light_controller: Licht wird ausgeschaltet, da kein aktiver Zeitplan vorhanden ist");
            achieve_light_intensity(0);
            continue;
        }

        // 5. Warten bis Startzeit erreicht ist
        if (timeSynchronized && !scheduleAdjusted)
        {
            LOG_INFO("task_light_controller: Überprüfen, ob die Startzeit erreicht wurde");
            if (!wait_for_start_time(&startTimeTm)) {
                LOG_INFO("task_light_controller: Aktuelle Zeit vor Startzeit. Licht wird ausgeschaltet.");
                achieve_light_intensity(0);
                continue;
            }

            // 6. Zeitplan anpassen basierend auf verstrichener Zeit
            struct tm currentTimeTm;
            if (!get_current_time(&currentTimeTm)) {
                LOG_ERROR("task_light_controller: Fehler beim Abrufen der aktuellen Zeit");
                continue;
            }

            time_t currentSeconds = mktime(&currentTimeTm);
            time_t startSeconds = mktime(&startTimeTm);
            time_t elapsedTime = currentSeconds - startSeconds;

            LOG_DEBUG("task_light_controller: Zeitplan wird basierend auf verstrichener Zeit angepasst: %ld Sekunden", (long)elapsedTime);

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

        // 7. Lichtzeitplan verarbeiten
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
    }
}

// 2. Manuelle Befehle prüfen und verarbeiten
static void handle_manual_override(void)
{
    static LightCommand receivedCommand;
    if (osMessageQueueGet(xLightCommandQueueHandle, &receivedCommand, NULL, 0) == osOK)
    {
    	LOG_WARN("task_light_controller: xLightCommandQueueHandle available");

    	if (receivedCommand.commandType == LIGHT_COMMAND_SET_INTENSITY)
        {
            achieve_light_intensity(receivedCommand.intensity);

            LOG_INFO("task_light_controller: Manueller Lichtbefehl empfangen: Setze Intensität auf %d", receivedCommand.intensity);
        }
        else
        {
            LOG_WARN("task_light_controller: Unbekannter Befehlstyp empfangen");
        }
    }
}





// 5. Warten bis Startzeit erreicht ist
static bool wait_for_start_time(struct tm *startTimeTm)
{
    struct tm currentTimeTm;
    if (!get_current_time(&currentTimeTm)) {
        LOG_ERROR("task_light_controller: Fehler beim Abrufen der aktuellen Zeit");
        return false;
    }

    // Vergleiche aktuelle Zeit mit Startzeit
    time_t currentSeconds = mktime(&currentTimeTm);
    time_t startSeconds = mktime(startTimeTm);

    char currentTimeStr[20];
    char startTimeStr[20];
    strftime(currentTimeStr, sizeof(currentTimeStr), "%Y-%m-%d %H:%M:%S", &currentTimeTm);
    strftime(startTimeStr, sizeof(startTimeStr), "%Y-%m-%d %H:%M:%S", startTimeTm);

    LOG_INFO("task_light_controller: Aktuelle Zeit: %s", currentTimeStr);
    LOG_INFO("task_light_controller: Startzeit:     %s", startTimeStr);

    if (difftime(currentSeconds, startSeconds) < 0) {
        LOG_INFO("task_light_controller: Aktuelle Zeit vor Startzeit. Warte...");
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
        LOG_ERROR("task_light_controller: Ungültige Argumente für adjust_schedule_based_on_elapsed_time");
        return;
    }

    time_t accumulatedTime = 0;

    // Durchlaufen aller Zeitpläne
    for (uint8_t i = 0; i < ledScheduleCount; i++) {
        LedSchedule *schedule = &ledSchedules[i];

        // Validierung der Schedule-Daten
        if (schedule->durationOn == 0 && schedule->durationOff == 0) {
            LOG_WARN("task_light_controller: Zeitplan %d hat Dauer von Null für EIN- und AUS-Phasen. Überspringe.", i);
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
                LOG_WARN("task_light_controller: Wiederholung %d von Zeitplan %d hat Dauer von Null. Überspringe.", rep, i);
                continue;
            }

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
                LOG_INFO("task_light_controller: Zeitplan angepasst auf Zeitplan %d, Wiederholung %d, Zustand %s, Phasendauer %lu ms",
                         *scheduleIndex, *repetitionIndex, (*currentState == LIGHT_ON) ? "EIN" : "AUS", *phaseDuration);
                return;
            }

            accumulatedTime += repetitionDuration;
        }
    }

    // Wenn die verstrichene Zeit alle Zeitpläne übersteigt
    *scheduleIndex = ledScheduleCount;
    *finished = true;
    LOG_INFO("task_light_controller: Alle Zeitpläne basierend auf verstrichener Zeit abgeschlossen");
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
        LOG_ERROR("task_light_controller: Ungültige Argumente für process_light_schedule");
        return;
    }

    if (*scheduleIndex < growConfig->ledScheduleCount) {
        LedSchedule *schedule = &growConfig->ledSchedules[*scheduleIndex];

        if (*repetitionIndex < schedule->repetition) {
            // Starten der Phase, falls nicht bereits laufend
            if (*phaseDuration == 0) {
                if (*currentState == LIGHT_OFF) {
                    // Licht einschalten
                    LOG_INFO("task_light_controller: Schalte Licht EIN (Zeitplan %d, Wiederholung %d)", *scheduleIndex, *repetitionIndex);
                    achieve_light_intensity(100);
                    *currentState = LIGHT_ON;
                    *phaseDuration = schedule->durationOn * 1000; // in Millisekunden
                    LOG_DEBUG("task_light_controller: LIGHT_ON-Phase für %lu ms gestartet", *phaseDuration);
                } else {
                    // Licht ausschalten
                    LOG_INFO("task_light_controller: Schalte Licht AUS (Zeitplan %d, Wiederholung %d)", *scheduleIndex, *repetitionIndex);
                    *lightIntensity = 0;
                    achieve_light_intensity(0);
                    *currentState = LIGHT_OFF;
                    *phaseDuration = schedule->durationOff * 1000; // in Millisekunden
                    (*repetitionIndex)++;
                    LOG_DEBUG("task_light_controller: LIGHT_OFF-Phase für %lu ms gestartet", *phaseDuration);
                }
                *phaseStartTime = osKernelGetTickCount();
            } else {
                // Überprüfe, ob die Phase abgeschlossen ist
                uint32_t elapsedTimeMs = osKernelGetTickCount() - *phaseStartTime;
                LOG_DEBUG("task_light_controller: Verstrichene Phasenzeit: %lu ms / %lu ms", elapsedTimeMs, *phaseDuration);
                if (elapsedTimeMs >= *phaseDuration) {
                    // Beende die aktuelle Phase
                    LOG_INFO("task_light_controller: Phasendauer abgeschlossen. Vorbereitung auf nächste Phase.");
                    *phaseDuration = 0; // Bereit für die nächste Phase
                }
            }
        } else {
            // Zum nächsten Zeitplan wechseln
            LOG_INFO("task_light_controller: Alle Wiederholungen für Zeitplan %d abgeschlossen. Wechsle zum nächsten Zeitplan.", *scheduleIndex);
            (*scheduleIndex)++;
            *repetitionIndex = 0;
            *currentState = LIGHT_OFF;
            *phaseDuration = 0;
            LOG_INFO("task_light_controller: Wechsle zum nächsten LED-Zeitplan (Index %d)", *scheduleIndex);
        }
    } else {
        // Alle Zeitpläne abgeschlossen
        LOG_INFO("task_light_controller: Alle LED-Zeitpläne abgeschlossen");
        *finished = true;

        // Sicherstellen, dass das Licht ausgeschaltet ist
        LOG_INFO("task_light_controller: Stelle sicher, dass das Licht ausgeschaltet ist.");
        *lightIntensity = 0;
        achieve_light_intensity(*lightIntensity);
    }
}

void UpdateLightControllerState(uint8_t lightIntensity)
{
    osMutexAcquire(gControllerStateMutexHandle, osWaitForever);

    gControllerState.lightIntensity = lightIntensity;
    LOG_DEBUG("task_light_controller: lightIntensity in gControllerState auf %d aktualisiert", lightIntensity);

    osEventFlagsSet(gControllerEventGroupHandle, LIGHT_INTENSITY_CHANGED_BIT);

    // Status-Update senden
    send_status_update(MESSAGE_TYPE_STATUS_UPDATE, DEVICE_CONTROLLER, TARGET_LIGHT_INTENSITY, lightIntensity);

    osMutexRelease(gControllerStateMutexHandle);
}

void ControlLight(uint8_t lightIntensity)
{
    LOG_DEBUG("task_light_controller: Sende neuen PWM-Wert an Hardware-Aufgabe");

    // Erstelle Hardware-Befehl
    HardwareCommand cmd;
    cmd.commandType = COMMAND_CONTROL_LIGHT;
    cmd.intensity = lightIntensity;

    // Sende Befehl an Hardware-Aufgabe
    osStatus_t status = osMessageQueuePut(xHardwareQueueHandle, &cmd, 0, 0);
    if (status != osOK) {
        LOG_ERROR("task_light_controller: Fehler beim Einfügen von HardwareCommand in die Warteschlange. Status: %d", status);
    }
}

void achieve_light_intensity(uint8_t intensity)
{
    LOG_DEBUG("task_light_controller: set LightIntensity to %d", intensity);

    osMutexAcquire(gControllerStateMutexHandle, osWaitForever);
    uint8_t currentIntensity = gControllerState.lightIntensity;
    osMutexRelease(gControllerStateMutexHandle);



    if (currentIntensity != intensity) {
        UpdateLightControllerState(intensity);
        ControlLight(intensity);
        LOG_INFO("task_light_controller: Lichtintensität auf %d geändert", intensity);
    }
    else {
        LOG_DEBUG("task_light_controller: Keine Änderung erforderlich. Aktuelle Intensität: %d, Gewünschte Intensität: %d",
                  currentIntensity, intensity);
    }
}
