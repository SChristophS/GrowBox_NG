/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "network.h"
#include "uart_redirect.h"
#include "wizchip_init.h"
#include "wizchip_conf.h"
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "socket.h"
#include "stdlib.h"
#include "portmacro.h"
#include "task.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */





bool connect_to_backend = false;
bool socket_connected = false;



/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi2;

UART_HandleTypeDef huart2;

/* Definitions for aliveTask */
osThreadId_t aliveTaskHandle;
const osThreadAttr_t aliveTask_attributes = {
  .name = "aliveTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for webSocketTask */
osThreadId_t webSocketTaskHandle;
const osThreadAttr_t webSocketTask_attributes = {
  .name = "webSocketTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal1,
};
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI2_Init(void);
void StartaliveTask(void *argument);
void StartwebSocketTask(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_SPI2_Init();
  /* USER CODE BEGIN 2 */
  printf("\t - W5x00 Project - \r\n");
  resetAssert();
  HAL_Delay(300);
  resetDeassert();
  HAL_Delay(300);



  initialize_network();
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of aliveTask */
  aliveTaskHandle = osThreadNew(StartaliveTask, NULL, &aliveTask_attributes);

  /* creation of webSocketTask */
  webSocketTaskHandle = osThreadNew(StartwebSocketTask, NULL, &webSocketTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  if (aliveTaskHandle == NULL) {
      printf("Failed to create aliveTask.\r\n");
  } else {
      printf("aliveTask created successfully.\r\n");
  }

  /* creation of webSocketTask */
  if (webSocketTaskHandle == NULL) {
      printf("Failed to create webSocketTask.\r\n");
  } else {
      printf("webSocketTask created successfully.\r\n");
  }
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    // Network connection handling code
    // Example: handling incoming connections or performing periodic tasks

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */
  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */
  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */
  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */
  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */
  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */
  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, ChipSelect_for_W5500_Pin|ResetPin_for_W5500_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ChipSelect_for_W5500_Pin ResetPin_for_W5500_Pin */
  GPIO_InitStruct.Pin = ChipSelect_for_W5500_Pin|ResetPin_for_W5500_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void apply_mask(uint8_t *data, size_t len, const uint8_t *mask_key) {
    for (size_t i = 0; i < len; i++) {
        data[i] ^= mask_key[i % WEBSOCKET_MASK_KEY_SIZE];
    }
}

void send_websocket_message(uint8_t socket, const char *message) {
	printf("send_websocket_message\r\n");
    size_t msg_len = strlen(message);
    uint8_t mask_key[WEBSOCKET_MASK_KEY_SIZE];
    for (int i = 0; i < WEBSOCKET_MASK_KEY_SIZE; i++) {
        mask_key[i] = rand() % 256;
    }

    uint8_t masked_message[msg_len];
    memcpy(masked_message, message, msg_len);
    apply_mask(masked_message, msg_len, mask_key);

    uint8_t frame[2 + WEBSOCKET_MASK_KEY_SIZE + msg_len];
    frame[0] = 0x81; // FIN bit set, opcode for text frame
    frame[1] = 0x80 | msg_len; // MASK bit set, payload length
    memcpy(frame + 2, mask_key, WEBSOCKET_MASK_KEY_SIZE);
    memcpy(frame + 2 + WEBSOCKET_MASK_KEY_SIZE, masked_message, msg_len);

    send(socket, frame, sizeof(frame));
}


/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartaliveTask */
/**
  * @brief  Function implementing the aliveTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartaliveTask */
void StartaliveTask(void *argument)
{
  /* USER CODE BEGIN 5 */

  /* Infinite loop */
  for(;;)
  {
    printf("ToggleLED\r\n");
	HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);

	//osDelay(999); // Warte 1 Sekunde
	vTaskDelay(1000 / portTICK_PERIOD_MS);

  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartwebSocketTask */
/**
* @brief Function implementing the webSocketTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartwebSocketTask */
void StartwebSocketTask(void *argument)
{
  /* USER CODE BEGIN StartwebSocketTask */
	printf("StartwebSocketTask\r\n");
	// otherwise the task freeze
    uint8_t *buf = (uint8_t *)malloc(DATA_BUF_SIZE);
    if (buf == NULL) {
        printf("Failed to allocate memory for buffer\n");
        return;
    }

  uint8_t destip[4] = {192, 168, 178, 25}; // Beispiel-IP-Adresse
  uint16_t destport = 8085; // port

  //uint32_t last_ping_time = 0;
  static uint16_t any_port = 50000;
  uint8_t currentSocketStatus = 0;

  int websocket_upgraded = 0;


  /* Infinite loop */
  for(;;)
  {

      currentSocketStatus = getSn_SR(SOCK_DHCP);
	  //printf("Status of Socket %d is: %d\r\n", SOCK_DHCP, currentSocketStatus);

	  switch (currentSocketStatus) {
	      case SOCK_CLOSED:
	         printf("%d:Socket closed, reopening...\r\n", SOCK_DHCP);

	         if((socket(SOCK_DHCP, Sn_MR_TCP, any_port++, 0x00)) != SOCK_DHCP)
	         {
	            if(any_port == 0xffff) any_port = 50000;
	         }
	         printf("%d:Socket opened\r\n", SOCK_DHCP);
	         break;

	      case SOCK_INIT:
	          printf("Socket is initialized.\r\n");

	          printf("%d:Try to connect to the %d.%d.%d.%d : %d\r\n", SOCK_DHCP, destip[0], destip[1], destip[2], destip[3], destport);

	          if(connect(SOCK_DHCP, destip, destport) != SOCK_OK){
	        	  printf("PROBLEM\r\n");
	          }

	          break;
	      case SOCK_LISTEN:
	          printf("Socket is in listen state.\n");
	          break;








	      case SOCK_ESTABLISHED:
	          printf("Socket is established.\r\n");



	          if (getSn_IR(SOCK_DHCP) & Sn_IR_CON) {
	              printf("%d: Connected to - %d.%d.%d.%d : %d\r\n", SOCK_DHCP, destip[0], destip[1], destip[2], destip[3], destport);
	              setSn_IR(SOCK_DHCP, Sn_IR_CON);
//	              // Upgrade auf WebSocket
//	              //upgrade_to_websocket(SOCK_DHCP);
	          }



              if (!websocket_upgraded) {
                  if (upgrade_to_websocket(SOCK_DHCP) == 0) {
                      printf("WebSocket upgrade successful.\n");
                      websocket_upgraded = 1; // Upgrade als durchgeführt markieren
                  } else {
                      printf("WebSocket upgrade failed.\n");
                      close(SOCK_DHCP);
                      free(buf);
                  }
              }




//              // Sende Ping-Nachricht
//              uint32_t current_time = osKernelSysTick();
//              if (current_time - last_ping_time >= PING_INTERVAL) {
//                  send_websocket_message(SOCK_DHCP, "123");
//                  last_ping_time = current_time;
//                  printf("PingTime: %lu\r\n", last_ping_time);
//              }


              // Empfang von WebSocket-Daten
              int32_t ret;
              uint16_t size = 0;

              if ((size = getSn_RX_RSR(SOCK_DHCP)) > 0) {
                  if (size > DATA_BUF_SIZE) size = DATA_BUF_SIZE;

                  ret = recv(SOCK_DHCP, buf, size);
                  if (ret <= 0) {
                      printf("Error receiving data. Socket closed.\n");
                      close(SOCK_DHCP);
                      free(buf);
                  }
                  size = (uint16_t) ret;

                  buf[size] = '\0'; // Nullterminierung
                  printf("Empfangene Nachricht: %s\n", buf);
              }


	          break;
	      case SOCK_CLOSE_WAIT:
	          printf("Socket is closing.\n");
	          break;
	      case SOCK_UDP:
	          printf("Socket is in UDP mode.\n");
	          break;
	      case SOCK_IPRAW:
	          printf("Socket is in IP RAW mode.\n");
	          break;
	      case SOCK_MACRAW:
	          printf("Socket is in MAC RAW mode.\n");
	          break;
	      default:
	          printf("Unknown socket status: %d\n", currentSocketStatus);
	          break;
	  }




	  vTaskDelay(1000 / portTICK_PERIOD_MS); // Vermeide eine enge Schleife

  }


//    int32_t result = loopback_tcpc(SOCK_DHCP, buf, destip, destport);
//
//    // Bearbeite das Ergebnis
//    if (result < 0) {
//        printf("Loopback-TCP-Client-Operation fehlgeschlagen mit Fehlercode: %ld\r\n", (long)result);
//        // Delay to avoid tight loop
//        osDelay(1000); // Warte 1 Sekunde vor dem erneuten Versuch
//    }

//    osDelay(100); // Vermeide eine enge Schleife

  /* USER CODE END StartwebSocketTask */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
