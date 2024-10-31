#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include "ds3231.h"
#include <stdint.h>
#include <time.h>

uint32_t calculate_elapsed_seconds(DS3231_Time *startTime, DS3231_Time *currentTime);

#endif // TIME_UTILS_H
