/* time_utils.c */

#include "time_utils.h"
#include "DS3231.h"
#include "logger.h"
#include <stdio.h>

bool parse_iso8601_datetime(const char *datetime_str, struct tm *tm_time) {
    int year, month, day, hour, min, sec;
    if (sscanf(datetime_str, "%4d-%2d-%2dT%2d:%2d:%2d",
               &year, &month, &day, &hour, &min, &sec) != 6) {
        return false;
    }

    tm_time->tm_year = year - 1900; // `tm_year` ist die Anzahl der Jahre seit 1900
    tm_time->tm_mon = month - 1;    // `tm_mon` ist 0-basiert (0 = Januar)
    tm_time->tm_mday = day;
    tm_time->tm_hour = hour;
    tm_time->tm_min = min;
    tm_time->tm_sec = sec;
    tm_time->tm_isdst = -1;         // Daylight Saving Time unbekannt

    return true;
}

bool get_current_time(struct tm *currentTime) {
    DS3231_Time rtcTime;
    if (!DS3231_GetTime(&rtcTime)) {
        LOG_ERROR("get_current_time: Failed to get time from RTC");
        return false;
    }

    currentTime->tm_year = rtcTime.year - 1900;
    currentTime->tm_mon = rtcTime.month - 1;
    currentTime->tm_mday = rtcTime.dayOfMonth;
    currentTime->tm_hour = rtcTime.hours;
    currentTime->tm_min = rtcTime.minutes;
    currentTime->tm_sec = rtcTime.seconds;
    currentTime->tm_isdst = -1;

    return true;
}

time_t tm_to_seconds(struct tm *tm_time) {
    struct tm temp_tm = *tm_time;
    return mktime(&temp_tm); // Wenn mktime zuverlässig ist, ansonsten eigene Implementierung
}

int compare_tm(struct tm *tm1, struct tm *tm2) {
    if (tm1->tm_year != tm2->tm_year)
        return tm1->tm_year - tm2->tm_year;
    if (tm1->tm_mon != tm2->tm_mon)
        return tm1->tm_mon - tm2->tm_mon;
    if (tm1->tm_mday != tm2->tm_mday)
        return tm1->tm_mday - tm2->tm_mday;
    if (tm1->tm_hour != tm2->tm_hour)
        return tm1->tm_hour - tm2->tm_hour;
    if (tm1->tm_min != tm2->tm_min)
        return tm1->tm_min - tm2->tm_min;
    return tm1->tm_sec - tm2->tm_sec;
}
