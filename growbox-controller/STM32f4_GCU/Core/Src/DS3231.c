#include "ds3231.h"
#include <string.h>
#include <time.h>

// Hilfsfunktionen zur BCD-Konvertierung
static uint8_t BCD_To_Dec(uint8_t val) {
    return ((val / 16 * 10) + (val % 16));
}

static uint8_t Dec_To_BCD(uint8_t val) {
    return ((val / 10 * 16) + (val % 10));
}

time_t ds3231_time_to_timestamp(DS3231_Time *time) {
    struct tm tm_time;
    tm_time.tm_year = time->year - 1900;
    tm_time.tm_mon = time->month - 1;
    tm_time.tm_mday = time->dayOfMonth;
    tm_time.tm_hour = time->hours;
    tm_time.tm_min = time->minutes;
    tm_time.tm_sec = time->seconds;
    tm_time.tm_isdst = -1;
    return mktime(&tm_time);
}

bool DS3231_SetTime(DS3231_Time *time) {
    uint8_t buffer[7];
    buffer[0] = Dec_To_BCD(time->seconds);
    buffer[1] = Dec_To_BCD(time->minutes);
    buffer[2] = Dec_To_BCD(time->hours);
    buffer[3] = Dec_To_BCD(time->dayOfWeek);
    buffer[4] = Dec_To_BCD(time->dayOfMonth);
    buffer[5] = Dec_To_BCD(time->month);
    buffer[6] = Dec_To_BCD(time->year - 2000);  // DS3231 speichert das Jahr als zweistellige Zahl

    if (HAL_I2C_Mem_Write(&hi2c2, DS3231_ADDRESS, 0x00, I2C_MEMADD_SIZE_8BIT, buffer, 7, 1000) != HAL_OK) {
        return false;
    }
    return true;
}

bool DS3231_GetTime(DS3231_Time *time) {
    uint8_t buffer[7];

    if (HAL_I2C_Mem_Read(&hi2c2, DS3231_ADDRESS, 0x00, I2C_MEMADD_SIZE_8BIT, buffer, 7, 1000) != HAL_OK) {
        return false;
    }

    time->seconds = BCD_To_Dec(buffer[0] & 0x7F);
    time->minutes = BCD_To_Dec(buffer[1]);
    time->hours = BCD_To_Dec(buffer[2] & 0x3F);
    time->dayOfWeek = BCD_To_Dec(buffer[3]);
    time->dayOfMonth = BCD_To_Dec(buffer[4]);
    time->month = BCD_To_Dec(buffer[5] & 0x1F);
    time->year = BCD_To_Dec(buffer[6]) + 2000;

    return true;
}
