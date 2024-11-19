/*
 * task_light_controller.h
 *
 *  Created on: Jun 26, 2024
 *      Author: chris
 */

#ifndef INC_TASK_LIGHT_CONTROLLER_H_
#define INC_TASK_LIGHT_CONTROLLER_H_

#include "stm32f4xx_hal.h"
#include <time.h>
#include <stdbool.h>
#include "globals.h"

extern TIM_HandleTypeDef htim10;

void UpdateLightControllerState(uint8_t lightIntensity);
void ControlLight(uint8_t lightIntensity);
void achieve_light_intensity(uint8_t intensity);
bool parse_iso8601_datetime(const char *datetime_str, struct tm *tm_time);
time_t get_current_timestamp(void);




void adjust_schedule_for_elapsed_time(
    LedSchedule *ledSchedules,
    uint8_t ledScheduleCount,
    time_t elapsedTime,
    uint8_t *scheduleIndex,
    uint8_t *repetitionIndex,
    LightState *currentState,
    uint32_t *phaseDuration,
    uint32_t *phaseStartTime,
    bool *finished);

// Neue Hilfsfunktionen


void process_light_schedule(
    GrowCycleConfig *growConfig,
    uint8_t *scheduleIndex,
    uint8_t *repetitionIndex,
    LightState *currentState,
    uint32_t *phaseStartTime,
    uint32_t *phaseDuration,
    uint8_t *lightIntensity,
    bool *finished
);

#endif /* INC_TASK_LIGHT_CONTROLLER_H_ */
