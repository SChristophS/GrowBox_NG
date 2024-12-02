/* task_network.h */

#ifndef TASK_NETWORK_H
#define TASK_NETWORK_H



#include <stdint.h>
#include "message_types.h"
#include <stdbool.h>

void StartNetworkTask(void *argument);


bool save_manual_mode(bool manualMode);
bool load_manual_mode(bool *manualMode);




#endif /* TASK_NETWORK_H */
