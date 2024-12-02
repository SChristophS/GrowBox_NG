#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <stdbool.h>
#include <time.h>

// Parsen eines ISO8601-Datumsstrings in eine struct tm
bool parse_iso8601_datetime(const char *datetime_str, struct tm *tm_time);

// Abrufen der aktuellen Zeit von der RTC
bool get_current_time(struct tm *currentTime);

// Konvertieren einer struct tm in Gesamtsekunden
time_t tm_to_seconds(struct tm *tm_time);

// Vergleich von zwei struct tm Zeiten
int compare_tm(struct tm *tm1, struct tm *tm2);

#endif // TIME_UTILS_H
