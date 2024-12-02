/* ds3231.h */

#ifndef DS3231_H
#define DS3231_H

#include <stdbool.h>
#include <stdint.h>
#include "main.h"

extern I2C_HandleTypeDef hi2c2;

// Struktur zum Halten der Zeitinformationen vom DS3231
typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t dayOfWeek;
    uint8_t dayOfMonth;
    uint8_t month;
    uint16_t year; // Vierstelliges Jahr
} DS3231_Time;

// Funktion-Prototypen
bool DS3231_GetTime(DS3231_Time *time);
bool DS3231_SetTime(DS3231_Time *time);

#endif // DS3231_H


