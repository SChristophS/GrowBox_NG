/* schedules.h */

#ifndef SCHEDULES_H
#define SCHEDULES_H

#include <stdint.h>
#include <stdbool.h>

// Definiere maximal mögliche Pläne
#define MAX_LED_SCHEDULES 10
#define MAX_WATERING_SCHEDULES 10

#define GROW_CYCLE_CONFIG_SIGNATURE 0xDEADBEEF

typedef struct __attribute__((packed)) {
    uint32_t durationOn;
    uint32_t durationOff;
    uint8_t repetition;
} LedSchedule;

typedef struct __attribute__((packed)) {
    uint32_t duration_full;
    uint32_t duration_empty;
    uint8_t repetition;
} WateringSchedule;



typedef struct __attribute__((packed)) {
    uint32_t signature;
    char startGrowTime[20]; // Platz für ISO8601-Zeitstempel
    uint8_t ledScheduleCount;
    LedSchedule ledSchedules[MAX_LED_SCHEDULES];
    uint8_t wateringScheduleCount;
    WateringSchedule wateringSchedules[MAX_WATERING_SCHEDULES];
} GrowCycleConfig;




#endif /* SCHEDULES_H */
