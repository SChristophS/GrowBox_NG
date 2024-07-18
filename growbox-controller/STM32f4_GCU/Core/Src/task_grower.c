#include "uart_redirect.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdio.h>
#include "at24cxx.h"
#include <stdbool.h>


void StartGrowerTask(void *argument)
{
  /* USER CODE BEGIN 5 */
	printf("StartGrowerTask.c: start\r\n");


  /* Infinite loop */



  for(;;)
  {
	  printf("task_grower.c:\tI'm alive\r\n");
	  vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  /* USER CODE END 5 */
}

