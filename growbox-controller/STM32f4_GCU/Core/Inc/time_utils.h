#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include "ds3231.h"
#include <stdint.h>
#include <time.h>

uint32_t calculate_elapsed_seconds(DS3231_Time *startTime, DS3231_Time *currentTime);
bool parse_iso8601_datetime(const char *datetime_str, struct tm *tm_time);

#endif // TIME_UTILS_H
