#ifndef TASK_STATE_MANAGER_H
#define TASK_STATE_MANAGER_H

#include "ds3231.h"
#include "schedules.h"
#include <stdbool.h>
#include <stdint.h>
#include "DS3231.h"  // Falls du DS3231_Time benötigst
#include "cmsis_os.h" // Falls du osMutex benötigst
#include <time.h>

#define EEPROM_AUTOMATIC_MODE_ADDR 		   0x0100
#define EEPROM_MANUAL_MODE_ADDR 		   0x0102
#define EEPROM_GROW_CYCLE_START_TIME_ADDR  0x0200
#define EEPROM_GROW_CYCLE_CONFIG_ADDR      0x0300



// Funktionsdeklarationen
bool load_grow_cycle_config(GrowCycleConfig *config, struct tm *startTimeTm, bool *timeSynchronized);
bool save_grow_cycle_config(GrowCycleConfig *config);
bool save_automatic_mode(bool automaticMode);
bool load_automatic_mode(bool *automaticMode);
bool save_grow_cycle_start_time(DS3231_Time time);
bool load_grow_cycle_start_time(DS3231_Time *time);
void InitializeGrowCycleConfig(void);
void print_grow_cycle_config(void);


// Weitere Funktionen...

#endif // TASK_STATE_MANAGER_H
