/*
 * DS3231.h
 *
 *  Created on: Oct 31, 2024
 *      Author: chris
 */

#ifndef DS3231_H
#define DS3231_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// I2C-Handler extern deklarieren
extern I2C_HandleTypeDef hi2c2;

// DS3231 I2C-Adresse
#define DS3231_ADDRESS 0x68 << 1  // Die Adresse muss um 1 Bit nach links geschoben werden

// Struktur für die Zeitangaben
typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t dayOfWeek;   // 1 = Sonntag, 2 = Montag, ..., 7 = Samstag
    uint8_t dayOfMonth;
    uint8_t month;
    uint16_t year;       // Vierstelliges Jahr
} DS3231_Time;

//
time_t ds3231_time_to_timestamp(DS3231_Time *time);
bool DS3231_SetTime(DS3231_Time *time);
bool DS3231_GetTime(DS3231_Time *time);
time_t get_current_time(void);
bool parse_iso8601_datetime(const char *datetime_str, struct tm *tm_time);

#endif // DS3231_H

