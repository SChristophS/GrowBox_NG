/* schedules.h */

#ifndef SCHEDULES_H
#define SCHEDULES_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_LED_SCHEDULES 10
#define MAX_WATERING_SCHEDULES 10
/* Weitere MAX_* Definitionen für andere Zeitpläne */

typedef struct __attribute__((packed)) {
    uint32_t durationOn;
    uint32_t durationOff;
    uint8_t repetition;
} LedSchedule;


typedef struct __attribute__((packed)) {
    char status1[10];
    uint32_t duration1;
    char status2[10];
    uint32_t duration2;
    uint8_t waterRepetitions;
} WateringSchedule;

#define GROW_CYCLE_CONFIG_SIGNATURE 0xDEADBEEF



/* Ähnliche Strukturen für temperatureTargets, humidityTargets, airFilterSchedules */

typedef struct __attribute__((packed)) {
    uint32_t signature;
    uint32_t totalGrowTime;
    uint8_t startFromHere;
    uint8_t ledScheduleCount;
    LedSchedule ledSchedules[MAX_LED_SCHEDULES];
    uint8_t wateringScheduleCount;
    WateringSchedule wateringSchedules[MAX_WATERING_SCHEDULES];
} GrowCycleConfig;

#endif /* SCHEDULES_H */
