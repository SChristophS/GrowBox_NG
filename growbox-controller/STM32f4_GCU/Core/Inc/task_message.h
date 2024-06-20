#ifndef TASK_MESSAGE_H
#define TASK_MESSAGE_H

#include "cmsis_os.h"

extern osMessageQueueId_t xMessageQueueHandle;

void StartMessageTask(void *argument);

#endif // TASK_MESSAGE_H
