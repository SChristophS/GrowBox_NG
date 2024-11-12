#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "uart_redirect.h"
#include <stdlib.h>

void StartAliveTask(void *argument)
{
  /* USER CODE BEGIN 5 */
	printf("task_alive.c:\t Task started\r\n");

  /* Infinite loop */
  for(;;)
  {
	printf("task_alive.c:\tToggleLED\r\n");

	HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_12);
	vTaskDelay(250 / portTICK_PERIOD_MS);
	HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_13);
	vTaskDelay(250 / portTICK_PERIOD_MS);
	HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_14);
	vTaskDelay(250 / portTICK_PERIOD_MS);
	HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_15);
	vTaskDelay(250 / portTICK_PERIOD_MS);
  }
  /* USER CODE END 5 */
}

