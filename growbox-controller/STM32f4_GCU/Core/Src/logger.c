/* logger.c */

#include "logger.h"
#include <time.h>
#include "globals.h"

/* Initialisiere das aktuelle Logging-Level */
LogLevel currentLogLevel = LOG_LEVEL_DEBUG;

void log_message(LogLevel level, const char *format, ...)
{
	 osMutexAcquire(gLoggerMutexHandle, osWaitForever);

    if (level < currentLogLevel) {
        return; // Nachricht nicht ausgeben
    }

    /* Zeitstempel hinzufügen */
    time_t now;
    time(&now);
    struct tm *timeinfo = localtime(&now);

    char timeBuffer[20];
    strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", timeinfo);

    /* Logging-Level als String */
    const char *levelStrings[] = { "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL" };
    const char *levelString = levelStrings[level];

    /* Farben für die verschiedenen Logging-Level */
    const char *colorReset = "\033[0m";
    const char *colorRed = "\033[31m";
    const char *colorYellow = "\033[33m";
    const char *colorGreen = "\033[32m";
    const char *colorCyan = "\033[36m";
    const char *colorMagenta = "\033[35m";

    const char *color;
    switch (level) {
        case LOG_LEVEL_DEBUG:
            color = colorCyan;
            break;
        case LOG_LEVEL_INFO:
            color = colorGreen;
            break;
        case LOG_LEVEL_WARNING:
            color = colorYellow;
            break;
        case LOG_LEVEL_ERROR:
            color = colorRed;
            break;
        case LOG_LEVEL_CRITICAL:
            color = colorMagenta;
            break;
        default:
            color = colorReset;
            break;
    }

    /* Nachricht formatieren */
    printf("%s[%s] %s: ", color, timeBuffer, levelString);

    /* Variable Argumentliste verarbeiten */
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    /* Farbe zurücksetzen und neue Zeile hinzufügen */
    printf("%s\r\n", colorReset);

    osMutexRelease(gLoggerMutexHandle);
}
