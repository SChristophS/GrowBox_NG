/* task_state_manager.c */

#include "task_state_manager.h"
#include "eeprom.h"
#include <string.h>
#include "schedules.h"
#include <stdio.h>
#include "uart_redirect.h"
#include "controller_state.h"
#include "main.h"
#include "globals.h"


bool load_automatic_mode(bool *automaticMode);
bool save_grow_cycle_config(GrowCycleConfig *config);
bool save_automatic_mode(bool automaticMode);
bool save_grow_cycle_start_time(DS3231_Time time);
bool load_grow_cycle_start_time(DS3231_Time *time);
void InitializeGrowCycleConfig();


void InitializeGrowCycleConfig() {
    // Andere Initialisierungen...

	 printf("task_state_manager.c: Running initial Grow config\r\n");

    // Lade automaticMode aus dem EEPROM
    bool storedAutomaticMode;
    if (load_automatic_mode(&storedAutomaticMode)) {
        printf("task_state_manager.c: Loaded automaticMode: %s\r\n", storedAutomaticMode ? "ON" : "OFF");
        osMutexAcquire(gGrowCycleConfigMutexHandle, osWaitForever);
        gGrowCycleConfig.automaticMode = storedAutomaticMode;
        osMutexRelease(gGrowCycleConfigMutexHandle);
    } else {
        // Falls Laden fehlschlägt, setzen wir einen Standardwert
        printf("task_state_manager.c: Failed to load automaticMode, defaulting to OFF\r\n");
        osMutexAcquire(gGrowCycleConfigMutexHandle, osWaitForever);
        gGrowCycleConfig.automaticMode = false;
        osMutexRelease(gGrowCycleConfigMutexHandle);
    }
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

    printf("task_state_manager.c: Set flag that new GrowCycle is available\r\n");

    // Konfiguration in die globale Variable kopieren
    osMutexAcquire(gGrowCycleConfigMutexHandle, osWaitForever);
    memcpy(&gGrowCycleConfig, config, sizeof(GrowCycleConfig));
    osMutexRelease(gGrowCycleConfigMutexHandle);

    // Event-Flag setzen, um Tasks über die neue Konfiguration zu informieren
    osEventFlagsSet(gControllerEventGroupHandle, NEW_GROW_CYCLE_CONFIG_AVAILABLE);


    printf("task_state_manager.c: Size of GrowCycleConfig: %lu bytes\r\n", (unsigned long)sizeof(GrowCycleConfig));


    return true;
}

bool load_automatic_mode(bool *automaticMode) {
    uint16_t address = EEPROM_AUTOMATIC_MODE_ADDR; // Gleiche Adresse wie beim Speichern
    printf("task_state_manager.c: Loading automaticMode from EEPROM at address 0x%04X\r\n", address);

    if (!EEPROM_Read(address, (uint8_t *)automaticMode, sizeof(bool))) {
        printf("task_state_manager.c: Failed to read automaticMode from EEPROM\r\n");
        return false;
    }

    printf("task_state_manager.c: automaticMode loaded successfully\r\n");
    return true;
}

bool save_automatic_mode(bool automaticMode) {
    uint16_t address = EEPROM_AUTOMATIC_MODE_ADDR; // Neue Adresse für automaticMode
    printf("task_state_manager.c: Saving automaticMode to EEPROM at address 0x%04X\r\n", address);

    if (!EEPROM_Write(address, (uint8_t *)&automaticMode, sizeof(bool))) {
        printf("task_state_manager.c: Failed to write automaticMode to EEPROM\r\n");
        return false;
    }

    printf("task_state_manager.c: automaticMode saved successfully\r\n");
    return true;
}

bool load_grow_cycle_config(GrowCycleConfig *config) {
    if (config == NULL) {
        printf("task_state_manager.c: load_grow_cycle_config - Invalid config pointer\r\n");
        return false;
    }

    printf("task_state_manager.c: Loading GrowCycleConfig from EEPROM at address 0x%04X\r\n", EEPROM_GROW_CYCLE_CONFIG_ADDR);

    // Lese die Daten aus dem EEPROM
    if (!EEPROM_Read(EEPROM_GROW_CYCLE_CONFIG_ADDR, (uint8_t *)config, sizeof(GrowCycleConfig))) {
        printf("task_state_manager.c: Failed to read GrowCycleConfig from EEPROM\r\n");
        return false;
    }

    printf("task_state_manager.c: Size of GrowCycleConfig expected: %lu bytes\r\n", (unsigned long)sizeof(GrowCycleConfig));


    // Debug-Ausgaben des geladenen Konfigurationsinhalts
    printf("task_state_manager.c: GrowCycleConfig loaded successfully\r\n");
    printf("task_state_manager.c: startFromHere: %s\r\n", config->startGrowTime);
    printf("task_state_manager.c: LED Schedule Count: %d\r\n", config->ledScheduleCount);

    for (uint8_t i = 0; i < config->ledScheduleCount; i++) {
        LedSchedule *schedule = &config->ledSchedules[i];
        printf("task_state_manager.c: LED Schedule %d: durationOn=%lu s, durationOff=%lu s, repetition=%d\r\n",
               i, (unsigned long)schedule->durationOn, (unsigned long)schedule->durationOff, schedule->repetition);
    }

    // Ähnliche Debug-Ausgaben für andere Zeitpläne (z.B. Bewässerungspläne)
    printf("task_state_manager.c: Watering Schedule Count: %d\r\n", config->wateringScheduleCount);


    for (uint8_t i = 0; i < config->wateringScheduleCount; i++) {
        WateringSchedule *schedule = &config->wateringSchedules[i];
        printf("task_state_manager.c: Watering Schedule %d: duration_full=%lu s, duration_empty=%lu s, repetition=%d\r\n",
               i, (unsigned long)schedule->duration_full, (unsigned long)schedule->duration_empty, schedule->repetition);
    }


    printf("task_state_manager.c: Set flag that new GrowCycle is available\r\n");

    // Konfiguration in die globale Variable kopieren
    osMutexAcquire(gGrowCycleConfigMutexHandle, osWaitForever);
    memcpy(&gGrowCycleConfig, config, sizeof(GrowCycleConfig));
    osMutexRelease(gGrowCycleConfigMutexHandle);

    // Event-Flag setzen, um Tasks über die neue Konfiguration zu informieren
    osEventFlagsSet(gControllerEventGroupHandle, NEW_GROW_CYCLE_CONFIG_AVAILABLE);



    return true;
}


bool save_grow_cycle_start_time(DS3231_Time time) {
    printf("task_state_manager.c: Saving GrowCycle start time to EEPROM at address 0x%04X\r\n", EEPROM_GROW_CYCLE_START_TIME_ADDR);

    if (!EEPROM_Write(EEPROM_GROW_CYCLE_START_TIME_ADDR, (uint8_t *)&time, sizeof(DS3231_Time))) {
        printf("task_state_manager.c: Failed to write GrowCycle start time to EEPROM\r\n");
        return false;
    }

    printf("task_state_manager.c: GrowCycle start time saved successfully\r\n");
    return true;
}

/**
 * Lädt die Startzeit des Grow Cycles aus dem EEPROM.
 *
 * @param time Zeiger auf die DS3231_Time-Struktur, in die die Startzeit geladen werden soll.
 * @return true bei Erfolg, false bei Fehler.
 */
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
