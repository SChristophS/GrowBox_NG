#include "time_utils.h"
#include <stdio.h>


bool parse_iso8601_datetime(const char *datetime_str, struct tm *tm_time)
{
    int year, month, day, hour, min, sec;
    if (sscanf(datetime_str, "%4d-%2d-%2dT%2d:%2d:%2d",
               &year, &month, &day, &hour, &min, &sec) != 6)
    {
        return false;
    }

    tm_time->tm_year = year - 1900; // `tm_year` ist die Anzahl der Jahre seit 1900
    tm_time->tm_mon = month - 1;    // `tm_mon` ist 0-basiert (0 = Januar)
    tm_time->tm_mday = day;
    tm_time->tm_hour = hour;
    tm_time->tm_min = min;
    tm_time->tm_sec = sec;
    tm_time->tm_isdst = -1; // Daylight Saving Time unbekannt

    return true;
}


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
