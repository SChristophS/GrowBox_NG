#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "uart_redirect.h"
#include <stdlib.h>
#include "at24cxx.h"

//extern I2C_HandleTypeDef hi2c2;
//#define EEPROM_I2C &hi2c2


/* USER CODE BEGIN PV */
bool writeStatus = false;
bool readStatus = false;
bool eraseStatus = false;
uint8_t  wData[] = "Hello World 123";
uint8_t  rData[25];
#define MEM_ADDR    0x00u


// UID auslesen und anzeigen
void GetSTM32UID(char *uidStr) {
    uint32_t uid[3];
    uid[0] = *(uint32_t *)0x1FFF7A10;
    uid[1] = *(uint32_t *)0x1FFF7A14;
    uid[2] = *(uint32_t *)0x1FFF7A18;

    if (sprintf(uidStr, "%08lX%08lX%08lX", uid[0], uid[1], uid[2]) < 0) {
        printf("task_network.c:\t - Error formatting UID string\r\n");
        return;
    }
}

void StartAliveTask(void *argument)
{
  /* USER CODE BEGIN 5 */
	printf("task_alive.c:\t Init\r\n");



	char uidStr[25];

	GetSTM32UID(uidStr);
	printf("task_alive.c:\t STM32 UID: %s\r\n", uidStr);

	printf("task_alive.c:\t EEPROM test start\r\n");



	if(at24_isConnected()){
		// at24_eraseChip can take more than 30 Sec
		eraseStatus = at24_eraseChip();
		vTaskDelay(10 / portTICK_PERIOD_MS);
		writeStatus = at24_write(MEM_ADDR,wData, 15, 100);
		vTaskDelay(10 / portTICK_PERIOD_MS);
		readStatus = at24_read(MEM_ADDR,rData, 15, 100);
		vTaskDelay(10 / portTICK_PERIOD_MS);


	}



	// Gelesene Daten via UART im Dezimalformat ausgeben
	printf("task_alive.c:\t Gelesen aus EEPROM DEZ:");
	for (int i = 0; i < 3; i++) {
	    printf(" %d", rData[i]);

	}
	printf("\r\n");

	printf("task_alive.c:\t Gelesen aus EEPROM Text: %s\r\n", rData);

	printf("task_alive.c:\t - EEPROM Test Ende\r\n");

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

