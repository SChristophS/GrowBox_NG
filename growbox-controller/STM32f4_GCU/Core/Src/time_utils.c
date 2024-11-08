#include "time_utils.h"

uint32_t calculate_elapsed_seconds(DS3231_Time *startTime, DS3231_Time *currentTime) {
    struct tm start_tm = {
        .tm_year = startTime->year - 1900,
        .tm_mon = startTime->month - 1,
        .tm_mday = startTime->dayOfMonth,
        .tm_hour = startTime->hours,
        .tm_min = startTime->minutes,
        .tm_sec = startTime->seconds
    };

    struct tm current_tm = {
        .tm_year = currentTime->year - 1900,
        .tm_mon = currentTime->month - 1,
        .tm_mday = currentTime->dayOfMonth,
        .tm_hour = currentTime->hours,
        .tm_min = currentTime->minutes,
        .tm_sec = currentTime->seconds
    };

    time_t start_epoch = mktime(&start_tm);
    time_t current_epoch = mktime(&current_tm);

    return (uint32_t)(current_epoch - start_epoch);
}
