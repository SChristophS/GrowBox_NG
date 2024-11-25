#include "cmsis_os.h"
#include "controller_state.h"
#include "uart_redirect.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "helper_websocket.h"
#include "task_network.h"
#include "globals.h"
#include "controller_state.h"


void StartWatcherTask(void *argument) {
    /* USER CODE BEGIN StartWatcherTask */
    /* Infinite loop */
    for (;;) {
        while (1) {


            // CPU bisschen entlasten damit die anderen Tasks auch genug Leistung haben
            //vTaskDelay(10000 / portTICK_PERIOD_MS);
        	osDelay(10000);
        }
    }
    /* USER CODE END StartWatcherTask */
}




