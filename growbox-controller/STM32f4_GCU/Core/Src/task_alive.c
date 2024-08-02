#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "uart_redirect.h"
#include <stdlib.h>


//extern I2C_HandleTypeDef hi2c2;
//#define EEPROM_I2C &hi2c2




void StartAliveTask(void *argument)
{
  /* USER CODE BEGIN 5 */
	printf("task_alive.c:\t Init\r\n");





  /* Infinite loop */



  for(;;)
  {
	  printf("task_alive.c:\tToggleLED\r\n");
	  		//HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
	  HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_12);
	  HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_13);
	  HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_14);
	  HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_15);

	  //osDelay(999); // Warte 1 Sekunde
	  vTaskDelay(1000 / portTICK_PERIOD_MS);



  }
  /* USER CODE END 5 */
}

