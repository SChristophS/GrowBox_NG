/* task_network.h */

#ifndef TASK_NETWORK_H
#define TASK_NETWORK_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "stdlib.h"

typedef struct {
    uint8_t message_type;
    uint8_t device;
    uint8_t target;
    uint8_t action;
    uint16_t value;
} MessageForWebSocket;

void StartNetworkTask(void *argument);

#endif /* TASK_NETWORK_H */
