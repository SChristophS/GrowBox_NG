/* task_state_manager.c */

#include "task_state_manager.h"
#include "eeprom.h"
#include <string.h>
#include "schedules.h"
#include <stdio.h>
#include "uart_redirect.h"

/* Beispieladressen im EEPROM */
#define EEPROM_GROW_CYCLE_CONFIG_ADDR      0x0100  // Adresse für GrowCycleConfig
#define EEPROM_GROW_CYCLE_START_TIME_ADDR  0x0200  // Adresse für Startzeit des Grow Cycles

/**
 * Speichert die GrowCycle-Konfiguration im EEPROM.
 *
 * @param config Zeiger auf die GrowCycleConfig-Struktur, die gespeichert werden soll.
 * @return true bei Erfolg, false bei Fehler.
 */
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
        printf("task_state_manager.c: Invalid signature in read-back data: expected 0x%08X, got 0x%08X\r\n",
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

    printf("task_state_manager.c: Size of GrowCycleConfig: %lu bytes\r\n", (unsigned long)sizeof(GrowCycleConfig));


    return true;
}


/**
 * Lädt die GrowCycle-Konfiguration aus dem EEPROM.
 *
 * @param config Zeiger auf die GrowCycleConfig-Struktur, in die die Daten geladen werden sollen.
 * @return true bei Erfolg, false bei Fehler.
 */
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
    printf("task_state_manager.c: totalGrowTime: %lu\r\n", (unsigned long)config->totalGrowTime);
    printf("task_state_manager.c: startFromHere: %d\r\n", config->startFromHere);
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
        printf("task_state_manager.c: Watering Schedule %d: status1=%s, duration1=%lu s, status2=%s, duration2=%lu s, waterRepetitions=%d\r\n",
               i, schedule->status1, (unsigned long)schedule->duration1, schedule->status2, (unsigned long)schedule->duration2, schedule->waterRepetitions);
    }



    return true;
}

/**
 * Speichert die Startzeit des Grow Cycles im EEPROM.
 *
 * @param time DS3231_Time-Struktur, die die Startzeit enthält.
 * @return true bei Erfolg, false bei Fehler.
 */
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
