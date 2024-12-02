/* logger.h */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdarg.h>

/* Definition der Logging-Level */
typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_CRITICAL
} LogLevel;

/* Globale Variable für das aktuelle Logging-Level */
extern LogLevel currentLogLevel;

/* Prototyp der Logging-Funktion */
void log_message(LogLevel level, const char *format, ...);

/* Makros für die verschiedenen Logging-Level */
#define LOG_DEBUG(fmt, ...) log_message(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  log_message(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  log_message(LOG_LEVEL_WARNING, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) log_message(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define LOG_CRITICAL(fmt, ...) log_message(LOG_LEVEL_CRITICAL, fmt, ##__VA_ARGS__)

#endif /* LOGGER_H */
