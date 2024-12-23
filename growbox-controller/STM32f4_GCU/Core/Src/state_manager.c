/* task_state_manager.c */

#include "state_manager.h"
#include "eeprom.h"
#include <string.h>
#include <stdio.h>
#include "uart_redirect.h"
#include "controller_state.h"
#include "main.h"
#include "globals.h"
#include "logger.h"
#include "time_utils.h"
#include "task_network.h"


bool save_grow_cycle_config(GrowCycleConfig *config);
bool save_grow_cycle_start_time(DS3231_Time time);
bool load_grow_cycle_start_time(DS3231_Time *time);
void InitializeGrowCycleConfig();
void print_grow_cycle_config();


void print_grow_cycle_config() {
    printf("task_state_manager.c: Current GrowCycleConfig:\r\n");

    // Zugriff auf gGrowCycleConfig sichern
    osMutexAcquire(gGrowCycleConfigMutexHandle, osWaitForever);

    printf("  StartGrowTime: %s\r\n", gGrowCycleConfig.startGrowTime);
    printf("  LED Schedule Count: %d\r\n", gGrowCycleConfig.ledScheduleCount);
    for (uint8_t i = 0; i < gGrowCycleConfig.ledScheduleCount; i++) {
        LedSchedule *schedule = &gGrowCycleConfig.ledSchedules[i];
        printf("    LED Schedule %d: durationOn=%lu s, durationOff=%lu s, repetition=%d\r\n",
               i, (unsigned long)schedule->durationOn, (unsigned long)schedule->durationOff, schedule->repetition);
    }

    printf("  Watering Schedule Count: %d\r\n", gGrowCycleConfig.wateringScheduleCount);
    for (uint8_t i = 0; i < gGrowCycleConfig.wateringScheduleCount; i++) {
        WateringSchedule *schedule = &gGrowCycleConfig.wateringSchedules[i];
        printf("    Watering Schedule %d: duration_full=%lu s, duration_empty=%lu s, repetition=%d\r\n",
               i, (unsigned long)schedule->duration_full, (unsigned long)schedule->duration_empty, schedule->repetition);
    }

    osMutexRelease(gGrowCycleConfigMutexHandle);

     // **Neu hinzugefügt: manualMode ausgeben**
    osMutexAcquire(gManualModeMutexHandle, osWaitForever);
    printf("  ManualMode: %s\r\n", manualMode ? "ON" : "OFF");
    osMutexRelease(gManualModeMutexHandle);
}


void InitializeGrowCycleConfig(void) {
    LOG_INFO("state_manager.c: Initialize Grow Configuration");

    bool storedManualMode;
    bool newConfigLoaded;
    GrowCycleConfig tempConfig;
    struct tm startTimeTm;
    bool timeSynchronized = false;

    if (load_grow_cycle_config(&tempConfig, &startTimeTm, &timeSynchronized)) {
        LOG_INFO("state_manager.c: Loaded GrowCycleConfig from EEPROM");

        // Kopiere die gültige Konfiguration in die globale Variable
        osMutexAcquire(gGrowCycleConfigMutexHandle, osWaitForever);
        memcpy(&gGrowCycleConfig, &tempConfig, sizeof(GrowCycleConfig));
        osMutexRelease(gGrowCycleConfigMutexHandle);

        // Setze die Verfügbarkeitsvariable
        osMutexAcquire(gConfigAvailableMutexHandle, osWaitForever);
        gConfigAvailable = true;
        osMutexRelease(gConfigAvailableMutexHandle);

        // Speichere startTimeTm und timeSynchronized in globalen Variablen
        osMutexAcquire(gStartTimeMutexHandle, osWaitForever);
        memcpy(&gStartTimeTm, &startTimeTm, sizeof(struct tm));
        gTimeSynchronized = timeSynchronized;
        osMutexRelease(gStartTimeMutexHandle);

        newConfigLoaded = true;
    } else {
        LOG_WARN("state_manager.c: Failed to load valid GrowCycleConfig, initializing with default values");
        newConfigLoaded = false;

        // Initialisiere gGrowCycleConfig mit Standardwerten
        osMutexAcquire(gGrowCycleConfigMutexHandle, osWaitForever);
        memset(&gGrowCycleConfig, 0, sizeof(GrowCycleConfig));
        gGrowCycleConfig.signature = GROW_CYCLE_CONFIG_SIGNATURE;
        // Hier kannst du weitere Standardwerte setzen
        osMutexRelease(gGrowCycleConfigMutexHandle);

        osMutexAcquire(gConfigAvailableMutexHandle, osWaitForever);
        gConfigAvailable = false;
        osMutexRelease(gConfigAvailableMutexHandle);
    }

    if (load_manual_mode(&storedManualMode)) {
        osMutexAcquire(gManualModeMutexHandle, osWaitForever);
        manualMode = storedManualMode;
        osMutexRelease(gManualModeMutexHandle);
        LOG_INFO("state_manager.c: Loaded manualMode: %s", manualMode ? "ON" : "OFF");
    } else {
        osMutexAcquire(gManualModeMutexHandle, osWaitForever);
        manualMode = false;
        osMutexRelease(gManualModeMutexHandle);
        LOG_WARN("state_manager.c: Failed to load manualMode, defaulting to OFF");
    }

    print_grow_cycle_config();

    if (newConfigLoaded){
        LOG_INFO("state_manager.c: New Configuration is loaded, set Flags");
        osEventFlagsSet(gControllerEventGroupHandle, NEW_GROW_CYCLE_CONFIG_AVAILABLE_WATER);
        osEventFlagsSet(gControllerEventGroupHandle, NEW_GROW_CYCLE_CONFIG_AVAILABLE_LIGHT);
    }

    LOG_INFO("state_manager.c: Initialization complete");
}


bool save_grow_cycle_config(GrowCycleConfig *config) {
    if (config == NULL) {
        printf("task_state_manager.c: save_grow_cycle_config - Invalid config pointer\r\n");
        return false;
    }

    printf("task_state_manager.c: Saving GrowCycleConfig to EEPROM at address 0x%04X\r\n", EEPROM_GROW_CYCLE_CONFIG_ADDR);

    // Optional: Berechne und speichere eine Prüfsumme oder Signatur, um die Datenintegrität zu überprüfen
    config->signature = GROW_CYCLE_CONFIG_SIGNATURE;

    printf("task_state_manager.c: Size of GrowCycleConfig: %lu bytes\r\n", (unsigned long)sizeof(GrowCycleConfig));

    // Speichere die Daten im EEPROM
    if (!EEPROM_Write(EEPROM_GROW_CYCLE_CONFIG_ADDR, (uint8_t *)config, sizeof(GrowCycleConfig))) {
        printf("task_state_manager.c: Failed to write GrowCycleConfig to EEPROM\r\n");
        return false;
    }

    printf("task_state_manager.c: GrowCycleConfig saved successfully\r\n");

    // **Neu hinzugefügt: Daten unmittelbar nach dem Speichern lesen und überprüfen**
    GrowCycleConfig readBackConfig;
    if (!EEPROM_Read(EEPROM_GROW_CYCLE_CONFIG_ADDR, (uint8_t *)&readBackConfig, sizeof(GrowCycleConfig))) {
        printf("task_state_manager.c: Failed to read back GrowCycleConfig from EEPROM\r\n");
        return false;
    }

    // Überprüfe die Signatur
    if (readBackConfig.signature != GROW_CYCLE_CONFIG_SIGNATURE) {
        printf("task_state_manager.c: Invalid signature in read-back data: expected 0x%08X, got 0x%08lX\r\n",
               GROW_CYCLE_CONFIG_SIGNATURE, readBackConfig.signature);
        return false;
    }

    // Vergleiche die gespeicherten und gelesenen Daten
    if (memcmp(config, &readBackConfig, sizeof(GrowCycleConfig)) != 0) {
        printf("task_state_manager.c: Mismatch between saved and read-back GrowCycleConfig\r\n");
        // Hier kannst du detailliertere Vergleiche durchführen und die Unterschiede ausgeben
        return false;
    } else {
        printf("task_state_manager.c: Verified that saved and read-back GrowCycleConfig are identical\r\n");
    }

    // Konfiguration in die globale Variable kopieren
    osMutexAcquire(gGrowCycleConfigMutexHandle, osWaitForever);
    memcpy(&gGrowCycleConfig, config, sizeof(GrowCycleConfig));
    osMutexRelease(gGrowCycleConfigMutexHandle);

    // **Setze gConfigAvailable auf true**
    osMutexAcquire(gConfigAvailableMutexHandle, osWaitForever);
    gConfigAvailable = true;
    osMutexRelease(gConfigAvailableMutexHandle);

    // Event-Flag setzen, um Tasks über die neue Konfiguration zu informieren
    //@todo: Brauche ich das noch?
    osEventFlagsSet(gControllerEventGroupHandle, NEW_GROW_CYCLE_CONFIG_AVAILABLE_LIGHT);
    osEventFlagsSet(gControllerEventGroupHandle, NEW_GROW_CYCLE_CONFIG_AVAILABLE_WATER);

    printf("task_state_manager.c: Size of GrowCycleConfig: %lu bytes\r\n", (unsigned long)sizeof(GrowCycleConfig));

    return true;
}

bool load_grow_cycle_config(GrowCycleConfig *config, struct tm *startTimeTm, bool *timeSynchronized) {
    if (config == NULL || startTimeTm == NULL || timeSynchronized == NULL) {
        printf("state_manager.c: load_grow_cycle_config - Invalid pointer(s)\r\n");
        return false;
    }

    LOG_INFO("state_manager.c: Loading GrowCycleConfig from EEPROM at address 0x%04X", EEPROM_GROW_CYCLE_CONFIG_ADDR);

    // Lese die Konfiguration aus dem EEPROM
    if (!EEPROM_Read(EEPROM_GROW_CYCLE_CONFIG_ADDR, (uint8_t *)config, sizeof(GrowCycleConfig))) {
        printf("state_manager.c: Failed to read GrowCycleConfig from EEPROM\r\n");
        return false;
    }

    // **Neu hinzugefügt: Überprüfung der Signatur**
    if (config->signature != GROW_CYCLE_CONFIG_SIGNATURE) {
        printf("state_manager.c: Invalid signature in EEPROM data. Expected 0x%08X, got 0x%08lX\r\n",
               GROW_CYCLE_CONFIG_SIGNATURE, config->signature);
        return false;
    }

    // Startzeit aus startGrowTime parsen
    struct tm start_tm;
    if (parse_iso8601_datetime(config->startGrowTime, &start_tm)) {
        // Kopiere den geparsten Zeitstempel in startTimeTm
        memcpy(startTimeTm, &start_tm, sizeof(struct tm));
        *timeSynchronized = true;
        LOG_INFO("state_manager.c: Parsed startGrowTime successfully");
    } else {
        LOG_ERROR("state_manager.c: Failed to parse startGrowTime");
        *timeSynchronized = false;
    }

    // Debug-Ausgaben des geladenen Konfigurationsinhalts
    LOG_INFO("state_manager.c: GrowCycleConfig loaded successfully");
    LOG_INFO("state_manager.c: startFromHere: %s", config->startGrowTime);
    LOG_INFO("state_manager.c: LED Schedule Count: %d", config->ledScheduleCount);

    for (uint8_t i = 0; i < config->ledScheduleCount; i++) {
        LedSchedule *schedule = &config->ledSchedules[i];
        LOG_INFO("state_manager.c: LED Schedule %d: durationOn=%lu s, durationOff=%lu s, repetition=%d",
               i, (unsigned long)schedule->durationOn, (unsigned long)schedule->durationOff, schedule->repetition);
    }

    // Ähnliche Debug-Ausgaben für andere Zeitpläne (z.B. Bewässerungspläne)
    LOG_INFO("state_manager.c: Watering Schedule Count: %d", config->wateringScheduleCount);

    for (uint8_t i = 0; i < config->wateringScheduleCount; i++) {
        WateringSchedule *schedule = &config->wateringSchedules[i];
        LOG_INFO("state_manager.c: Watering Schedule %d: duration_full=%lu s, duration_empty=%lu s, repetition=%d",
               i, (unsigned long)schedule->duration_full, (unsigned long)schedule->duration_empty, schedule->repetition);
    }

    // Konfiguration in die globale Variable kopieren
    osMutexAcquire(gGrowCycleConfigMutexHandle, osWaitForever);
    memcpy(&gGrowCycleConfig, config, sizeof(GrowCycleConfig));
    osMutexRelease(gGrowCycleConfigMutexHandle);

    // set gConfigAvailable to true
    osMutexAcquire(gConfigAvailableMutexHandle, osWaitForever);
    gConfigAvailable = true;
    osMutexRelease(gConfigAvailableMutexHandle);

    return true;
}


bool save_grow_cycle_start_time(DS3231_Time time) {
    printf("task_state_manager.c: Saving GrowCycle start time to EEPROM at address 0x%04X\r\n", EEPROM_GROW_CYCLE_START_TIME_ADDR);

    if (!EEPROM_Write(EEPROM_GROW_CYCLE_START_TIME_ADDR, (uint8_t *)&time, sizeof(DS3231_Time))) {
        printf("task_state_manager.c: Failed to write GrowCycle start time to EEPROM\r\n");
        return false;
    }

    printf("task_state_manager.c: GrowCycle start time saved successfully\r\n");

    // Event-Flag setzen, um Tasks über die neue Konfiguration zu informieren
    osEventFlagsSet(gControllerEventGroupHandle, NEW_GROW_CYCLE_CONFIG_AVAILABLE_LIGHT);
    osEventFlagsSet(gControllerEventGroupHandle, NEW_GROW_CYCLE_CONFIG_AVAILABLE_WATER);

    return true;
}

bool load_grow_cycle_start_time(DS3231_Time *time) {
    if (time == NULL) {
        printf("task_state_manager.c: load_grow_cycle_start_time - Invalid time pointer\r\n");
        return false;
    }

    printf("task_state_manager.c: Loading GrowCycle start time from EEPROM at address 0x%04X\r\n", EEPROM_GROW_CYCLE_START_TIME_ADDR);

    if (!EEPROM_Read(EEPROM_GROW_CYCLE_START_TIME_ADDR, (uint8_t *)time, sizeof(DS3231_Time))) {
        printf("task_state_manager.c: Failed to read GrowCycle start time from EEPROM\r\n");
        return false;
    }

    printf("task_state_manager.c: GrowCycle start time loaded successfully\r\n");
    printf("task_state_manager.c: Start Time - %04d-%02d-%02d %02d:%02d:%02d\r\n",
           time->year, time->month, time->dayOfMonth, time->hours, time->minutes, time->seconds);

    return true;
}
