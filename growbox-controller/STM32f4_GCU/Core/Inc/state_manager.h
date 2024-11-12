#ifndef TASK_STATE_MANAGER_H
#define TASK_STATE_MANAGER_H

#include "ds3231.h"
#include "schedules.h"
#include <stdbool.h>
#include <stdint.h>

#define EEPROM_AUTOMATIC_MODE_ADDR 		   0x0100
#define EEPROM_GROW_CYCLE_START_TIME_ADDR  0x0200
#define EEPROM_GROW_CYCLE_CONFIG_ADDR      0x0300

bool save_grow_cycle_config(GrowCycleConfig *config);
bool load_grow_cycle_config(GrowCycleConfig *config);

bool save_grow_cycle_start_time(DS3231_Time time);
bool load_grow_cycle_start_time(DS3231_Time *time);

void InitializeGrowCycleConfig();
bool save_automatic_mode(bool automaticMode);
// Weitere Funktionen...

#endif // TASK_STATE_MANAGER_H
