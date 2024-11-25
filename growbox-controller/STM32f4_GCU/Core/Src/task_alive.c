#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "uart_redirect.h"
#include <stdlib.h>
#include "globals.h"
#include "logger.h"

// Maximale erlaubte Zeit ohne Heartbeat in Millisekunden
#define HEARTBEAT_TIMEOUT_MS 3000
#define HEARBEATCHECK_INTERVAL 1000

void StartAliveTask(void *argument)
{
  /* USER CODE BEGIN 5 */
	LOG_INFO("task_alive:\tTask started");

    // Registriere den Heartbeat
    if (!RegisterTaskHeartbeat("task_alive")) {
        LOG_ERROR("task_alive: Failed to register heartbeat");
        Error_Handler();
    }

  /* Infinite loop */
  for(;;)
  {
	LOG_INFO("task_alive:\tAliveTask is alive");
	osDelay(HEARBEATCHECK_INTERVAL);

    // Heartbeat für MonitorTask aktualisieren
    UpdateTaskHeartbeat("task_alive");

	HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_12);
	HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_13);
	HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_14);
	HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_15);


    osMutexAcquire(gHeartbeatMutexHandle, osWaitForever);
    uint32_t current_tick = HAL_GetTick();
    for (int i = 0; i < MAX_MONITORED_TASKS; i++) {
        if (task_heartbeats[i].task_name != NULL) {
            uint32_t elapsed = current_tick - task_heartbeats[i].last_heartbeat;
            if (elapsed > HEARTBEAT_TIMEOUT_MS) {
                LOG_ERROR("MonitorTask: Task %s has not responded for %lu ms",
                          task_heartbeats[i].task_name, (unsigned long)elapsed);
                // Hier können Sie Maßnahmen ergreifen, z.B. Neustart der Task oder Systemneustart
            }
        }
    }
    osMutexRelease(gHeartbeatMutexHandle);


	//vTaskDelay(HEARBEATCHECK_INTERVAL / portTICK_PERIOD_MS);

  }
  /* USER CODE END 5 */
}

