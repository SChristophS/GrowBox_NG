#ifndef LED_CYCLES_H
#define LED_CYCLES_H

#include <stdint.h>
#include <stddef.h>

#define MAX_CYCLES 20

typedef struct {
    uint16_t durationOn;
    uint16_t durationOff;
    uint8_t ledRepetitions;
} LEDCycle;

typedef struct {
    uint8_t numCycles;
    LEDCycle cycles[10]; // Annahme: maximal 10 Zyklen
} LEDCycleData;


#endif // LED_CYCLES_H
