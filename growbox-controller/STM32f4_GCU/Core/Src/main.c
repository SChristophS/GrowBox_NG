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
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "socket.h"
#include "stdlib.h"
#include "math.h"

/* other */
#include "globals.h"

#include "controller_state.h"
#include "state_manager.h"
#include "uart_redirect.h"
#include "wizchip_init.h"
#include "logger.h"
#include "helper_websocket.h"

/* Tasks */
#include "task_alive.h"
#include "task_hardware.h"
#include "task_light_controller.h"
#include "task_network.h"
#include "task_sensor.h"
#include "task_water_controller.h"



/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
typedef StaticQueue_t osStaticMessageQDef_t;
typedef StaticSemaphore_t osStaticMutexDef_t;
typedef StaticEventGroup_t osStaticEventGroupDef_t;
/* USER CODE BEGIN PTD */



/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c2;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim6;
TIM_HandleTypeDef htim10;

UART_HandleTypeDef huart2;

/* Definitions for AliveTask */
osThreadId_t AliveTaskHandle;
uint32_t AliveTaskBuffer[ 512 ];
osStaticThreadDef_t AliveTaskControlBlock;
const osThreadAttr_t AliveTask_attributes = {
  .name = "AliveTask",
  .cb_mem = &AliveTaskControlBlock,
  .cb_size = sizeof(AliveTaskControlBlock),
  .stack_mem = &AliveTaskBuffer[0],
  .stack_size = sizeof(AliveTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for NetworkTask */
osThreadId_t NetworkTaskHandle;
const osThreadAttr_t NetworkTask_attributes = {
  .name = "NetworkTask",
  .stack_size = 3000 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for WaterController */
osThreadId_t WaterControllerHandle;
uint32_t WaterControllerBuffer[ 1024 ];
osStaticThreadDef_t WaterControllerControlBlock;
const osThreadAttr_t WaterController_attributes = {
  .name = "WaterController",
  .cb_mem = &WaterControllerControlBlock,
  .cb_size = sizeof(WaterControllerControlBlock),
  .stack_mem = &WaterControllerBuffer[0],
  .stack_size = sizeof(WaterControllerBuffer),
  .priority = (osPriority_t) osPriorityNormal1,
};
/* Definitions for LightTask */
osThreadId_t LightTaskHandle;
uint32_t LightTaskBuffer[ 1024 ];
osStaticThreadDef_t LightTaskControlBlock;
const osThreadAttr_t LightTask_attributes = {
  .name = "LightTask",
  .cb_mem = &LightTaskControlBlock,
  .cb_size = sizeof(LightTaskControlBlock),
  .stack_mem = &LightTaskBuffer[0],
  .stack_size = sizeof(LightTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal2,
};
/* Definitions for SensorTask */
osThreadId_t SensorTaskHandle;
uint32_t SensorTaskBuffer[ 512 ];
osStaticThreadDef_t SensorTaskControlBlock;
const osThreadAttr_t SensorTask_attributes = {
  .name = "SensorTask",
  .cb_mem = &SensorTaskControlBlock,
  .cb_size = sizeof(SensorTaskControlBlock),
  .stack_mem = &SensorTaskBuffer[0],
  .stack_size = sizeof(SensorTaskBuffer),
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for HardwareTask */
osThreadId_t HardwareTaskHandle;
uint32_t HardwareTaskBuffer[ 512 ];
osStaticThreadDef_t HardwareTaskControlBlock;
const osThreadAttr_t HardwareTask_attributes = {
  .name = "HardwareTask",
  .cb_mem = &HardwareTaskControlBlock,
  .cb_size = sizeof(HardwareTaskControlBlock),
  .stack_mem = &HardwareTaskBuffer[0],
  .stack_size = sizeof(HardwareTaskBuffer),
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for xWaterControllerQueue */
osMessageQueueId_t xWaterControllerQueueHandle;
uint8_t xWaterControllerQueueBuffer[ 1 * sizeof( uint8_t ) ];
osStaticMessageQDef_t xWaterControllerQueueControlBlock;
const osMessageQueueAttr_t xWaterControllerQueue_attributes = {
  .name = "xWaterControllerQueue",
  .cb_mem = &xWaterControllerQueueControlBlock,
  .cb_size = sizeof(xWaterControllerQueueControlBlock),
  .mq_mem = &xWaterControllerQueueBuffer,
  .mq_size = sizeof(xWaterControllerQueueBuffer)
};
/* Definitions for xLightControllerQueue */
osMessageQueueId_t xLightControllerQueueHandle;
uint8_t xLightControllerQueueBuffer[ 1 * sizeof( uint8_t ) ];
osStaticMessageQDef_t xLightControllerQueueControlBlock;
const osMessageQueueAttr_t xLightControllerQueue_attributes = {
  .name = "xLightControllerQueue",
  .cb_mem = &xLightControllerQueueControlBlock,
  .cb_size = sizeof(xLightControllerQueueControlBlock),
  .mq_mem = &xLightControllerQueueBuffer,
  .mq_size = sizeof(xLightControllerQueueBuffer)
};
/* Definitions for xWebSocketQueue */
osMessageQueueId_t xWebSocketQueueHandle;
//uint8_t xWebSocketQueueBuffer[ 3 * 6 ];
MessageForWebSocket* xWebSocketQueueBuffer[ 3 ];
osStaticMessageQDef_t xWebSocketQueueControlBlock;
const osMessageQueueAttr_t xWebSocketQueue_attributes = {
  .name = "xWebSocketQueue",
  .cb_mem = &xWebSocketQueueControlBlock,
  .cb_size = sizeof(xWebSocketQueueControlBlock),
  .mq_mem = &xWebSocketQueueBuffer,
  .mq_size = sizeof(xWebSocketQueueBuffer)
};
/* Definitions for xHardwareQueue */
osMessageQueueId_t xHardwareQueueHandle;
uint8_t xHardwareQueueBuffer[ 2 * 8 ];
osStaticMessageQDef_t xHardwareQueueControlBlock;
const osMessageQueueAttr_t xHardwareQueue_attributes = {
  .name = "xHardwareQueue",
  .cb_mem = &xHardwareQueueControlBlock,
  .cb_size = sizeof(xHardwareQueueControlBlock),
  .mq_mem = &xHardwareQueueBuffer,
  .mq_size = sizeof(xHardwareQueueBuffer)
};
/* Definitions for xWaterCommandQueue */
osMessageQueueId_t xWaterCommandQueueHandle;
uint8_t xWaterCommandQueueBuffer[ 3 * 4 ];
osStaticMessageQDef_t xWaterCommandQueueControlBlock;
const osMessageQueueAttr_t xWaterCommandQueue_attributes = {
  .name = "xWaterCommandQueue",
  .cb_mem = &xWaterCommandQueueControlBlock,
  .cb_size = sizeof(xWaterCommandQueueControlBlock),
  .mq_mem = &xWaterCommandQueueBuffer,
  .mq_size = sizeof(xWaterCommandQueueBuffer)
};
/* Definitions for xLightCommandQueue */
osMessageQueueId_t xLightCommandQueueHandle;
uint8_t xLightCommandQueueBuffer[ 3 * 4 ];
osStaticMessageQDef_t xLightCommandQueueControlBlock;
const osMessageQueueAttr_t xLightCommandQueue_attributes = {
  .name = "xLightCommandQueue",
  .cb_mem = &xLightCommandQueueControlBlock,
  .cb_size = sizeof(xLightCommandQueueControlBlock),
  .mq_mem = &xLightCommandQueueBuffer,
  .mq_size = sizeof(xLightCommandQueueBuffer)
};
/* Definitions for gControllerStateMutex */
osMutexId_t gControllerStateMutexHandle;
osStaticMutexDef_t gControllerStateMutexControlBlock;
const osMutexAttr_t gControllerStateMutex_attributes = {
  .name = "gControllerStateMutex",
  .cb_mem = &gControllerStateMutexControlBlock,
  .cb_size = sizeof(gControllerStateMutexControlBlock),
};
/* Definitions for gGrowCycleConfigMutex */
osMutexId_t gGrowCycleConfigMutexHandle;
osStaticMutexDef_t gGrowCycleConfigMutexControlBlock;
const osMutexAttr_t gGrowCycleConfigMutex_attributes = {
  .name = "gGrowCycleConfigMutex",
  .cb_mem = &gGrowCycleConfigMutexControlBlock,
  .cb_size = sizeof(gGrowCycleConfigMutexControlBlock),
};
/* Definitions for gEepromMutex */
osMutexId_t gEepromMutexHandle;
osStaticMutexDef_t gEepromMutexControlBlock;
const osMutexAttr_t gEepromMutex_attributes = {
  .name = "gEepromMutex",
  .cb_mem = &gEepromMutexControlBlock,
  .cb_size = sizeof(gEepromMutexControlBlock),
};
/* Definitions for gLoggerMutex */
osMutexId_t gLoggerMutexHandle;
osStaticMutexDef_t gLoggerMutexControlBlock;
const osMutexAttr_t gLoggerMutex_attributes = {
  .name = "gLoggerMutex",
  .cb_mem = &gLoggerMutexControlBlock,
  .cb_size = sizeof(gLoggerMutexControlBlock),
};
/* Definitions for gConfigAvailableMutex */
osMutexId_t gConfigAvailableMutexHandle;
osStaticMutexDef_t gConfigAvailableMutexControlBlock;
const osMutexAttr_t gConfigAvailableMutex_attributes = {
  .name = "gConfigAvailableMutex",
  .cb_mem = &gConfigAvailableMutexControlBlock,
  .cb_size = sizeof(gConfigAvailableMutexControlBlock),
};
/* Definitions for gStartTimeMutex */
osMutexId_t gStartTimeMutexHandle;
osStaticMutexDef_t gStartTimeMutexControlBlock;
const osMutexAttr_t gStartTimeMutex_attributes = {
  .name = "gStartTimeMutex",
  .cb_mem = &gStartTimeMutexControlBlock,
  .cb_size = sizeof(gStartTimeMutexControlBlock),
};
/* Definitions for gMessagePoolMutex */
osMutexId_t gMessagePoolMutexHandle;
osStaticMutexDef_t gMessagePoolMutexControlBlock;
const osMutexAttr_t gMessagePoolMutex_attributes = {
  .name = "gMessagePoolMutex",
  .cb_mem = &gMessagePoolMutexControlBlock,
  .cb_size = sizeof(gMessagePoolMutexControlBlock),
};
/* Definitions for gHeartbeatMutex */
osMutexId_t gHeartbeatMutexHandle;
osStaticMutexDef_t gHeartbeatMutexControlBlock;
const osMutexAttr_t gHeartbeatMutex_attributes = {
  .name = "gHeartbeatMutex",
  .cb_mem = &gHeartbeatMutexControlBlock,
  .cb_size = sizeof(gHeartbeatMutexControlBlock),
};
/* Definitions for gManualModeMutex */
osMutexId_t gManualModeMutexHandle;
osStaticMutexDef_t gManualModeMutexControlBlock;
const osMutexAttr_t gManualModeMutex_attributes = {
  .name = "gManualModeMutex",
  .cb_mem = &gManualModeMutexControlBlock,
  .cb_size = sizeof(gManualModeMutexControlBlock),
};
/* Definitions for gControllerEventGroup */
osEventFlagsId_t gControllerEventGroupHandle;
osStaticEventGroupDef_t gControllerEventGroupControlBlock;
const osEventFlagsAttr_t gControllerEventGroup_attributes = {
  .name = "gControllerEventGroup",
  .cb_mem = &gControllerEventGroupControlBlock,
  .cb_size = sizeof(gControllerEventGroupControlBlock),
};
/* Definitions for INITIALIZATION_COMPLETE */
osEventFlagsId_t INITIALIZATION_COMPLETEHandle;
osStaticEventGroupDef_t INITIALIZATION_COMPLETEControlBlock;
const osEventFlagsAttr_t INITIALIZATION_COMPLETE_attributes = {
  .name = "INITIALIZATION_COMPLETE",
  .cb_mem = &INITIALIZATION_COMPLETEControlBlock,
  .cb_size = sizeof(INITIALIZATION_COMPLETEControlBlock),
};
/* USER CODE BEGIN PV */

// globale Variablen
ControllerState gControllerState;
GrowCycleConfig gGrowCycleConfig;
char uidStr[25];
bool gConfigAvailable;
struct tm gStartTimeTm;
bool gTimeSynchronized;
bool is_registered = false;


bool manualMode = false;

/* Definitions for xWebSocketQueue */
#define MESSAGE_POINTER_SIZE sizeof(MessageForWebSocket*)


// Heartbeat-Daten (definiert global)
TaskHeartbeat task_heartbeats[MAX_MONITORED_TASKS];


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI2_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM6_Init(void);
static void MX_TIM10_Init(void);
static void MX_I2C2_Init(void);
void StartAliveTask(void *argument);
void StartNetworkTask(void *argument);
void StartWaterControllerTask(void *argument);
void StartLightTask(void *argument);
void StartSensorTask(void *argument);
void StartHardwareTask(void *argument);

/* USER CODE BEGIN PFP */
void InitControllerState(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// UID auslesen und anzeigen
void GetSTM32UID(char *uidStr) {
    uint32_t uid[3];
    uid[0] = *(uint32_t *)0x1FFF7A10;
    uid[1] = *(uint32_t *)0x1FFF7A14;
    uid[2] = *(uint32_t *)0x1FFF7A18;

    if (sprintf(uidStr, "%08lX%08lX%08lX", uid[0], uid[1], uid[2]) < 0) {
        LOG_INFO("main.c:\t - Error formatting UID string");
        return;
    }

    LOG_INFO("main.c:\t STM32 UID ausgelesen und gespeichert: %s", uidStr);

}



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
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  MX_SPI2_Init();
  MX_TIM3_Init();
  MX_TIM6_Init();
  MX_TIM10_Init();
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */

  LOG_INFO("main.c:\t NextGeneration Growbox Project");



  resetAssert();
  HAL_Delay(300);
  resetDeassert();
  HAL_Delay(300);

  // Initialisieren des Nachrichtenpools und des zugehörigen Mutexes
  initialize_message_pool();




  // Initialisiere gControllerState
  gControllerState.wasserbeckenZustand = false;
  gControllerState.pumpeZulauf = false;
  gControllerState.pumpeAblauf = false;
  gControllerState.sensorOben = false;
  gControllerState.sensorUnten = false;
  gControllerState.lightIntensity = 0;


  GetSTM32UID(uidStr);



  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();
  /* Create the mutex(es) */
  /* creation of gControllerStateMutex */
  gControllerStateMutexHandle = osMutexNew(&gControllerStateMutex_attributes);

  /* creation of gGrowCycleConfigMutex */
  gGrowCycleConfigMutexHandle = osMutexNew(&gGrowCycleConfigMutex_attributes);

  /* creation of gEepromMutex */
  gEepromMutexHandle = osMutexNew(&gEepromMutex_attributes);

  /* creation of gLoggerMutex */
  gLoggerMutexHandle = osMutexNew(&gLoggerMutex_attributes);

  /* creation of gConfigAvailableMutex */
  gConfigAvailableMutexHandle = osMutexNew(&gConfigAvailableMutex_attributes);

  /* creation of gStartTimeMutex */
  gStartTimeMutexHandle = osMutexNew(&gStartTimeMutex_attributes);

  /* creation of gMessagePoolMutex */
  gMessagePoolMutexHandle = osMutexNew(&gMessagePoolMutex_attributes);

  /* creation of gHeartbeatMutex */
  gHeartbeatMutexHandle = osMutexNew(&gHeartbeatMutex_attributes);

  /* creation of gManualModeMutex */
  gManualModeMutexHandle = osMutexNew(&gManualModeMutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of xWaterControllerQueue */
  xWaterControllerQueueHandle = osMessageQueueNew (1, sizeof(uint8_t), &xWaterControllerQueue_attributes);

  /* creation of xLightControllerQueue */
  xLightControllerQueueHandle = osMessageQueueNew (1, sizeof(uint8_t), &xLightControllerQueue_attributes);

  /* creation of xWebSocketQueue */
  xWebSocketQueueHandle = osMessageQueueNew (3, sizeof(MessageForWebSocket*), &xWebSocketQueue_attributes);

  /* creation of xHardwareQueue */
  xHardwareQueueHandle = osMessageQueueNew (2, 8, &xHardwareQueue_attributes);

  /* creation of xWaterCommandQueue */
  xWaterCommandQueueHandle = osMessageQueueNew (3, 4, &xWaterCommandQueue_attributes);

  /* creation of xLightCommandQueue */
  xLightCommandQueueHandle = osMessageQueueNew (3, 4, &xLightCommandQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  if (xWaterControllerQueueHandle == NULL) {
        printf("main.c:\t Error: Failed to create xWaterControllerQueue\r\n");
    }

  if (xLightControllerQueueHandle == NULL) {
      printf("main.c:\t Error: Failed to create xLightControllerQueue\r\n");
  }

  if (xHardwareQueueHandle == NULL) {
       printf("main.c:\t Error: Failed to create xAutoGrowQueue\r\n");
   }

  if (xWaterCommandQueueHandle == NULL) {
       printf("main.c:\t Error: Failed to create xWaterCommandQueue\r\n");
   }

  if (xLightCommandQueueHandle == NULL) {
       printf("main.c:\t Error: Failed to create xLightCommandQueue\r\n");
   }

  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of AliveTask */
  AliveTaskHandle = osThreadNew(StartAliveTask, NULL, &AliveTask_attributes);

  /* creation of NetworkTask */
  NetworkTaskHandle = osThreadNew(StartNetworkTask, NULL, &NetworkTask_attributes);

  /* creation of WaterController */
  WaterControllerHandle = osThreadNew(StartWaterControllerTask, NULL, &WaterController_attributes);

  /* creation of LightTask */
  LightTaskHandle = osThreadNew(StartLightTask, NULL, &LightTask_attributes);

  /* creation of SensorTask */
  SensorTaskHandle = osThreadNew(StartSensorTask, NULL, &SensorTask_attributes);

  /* creation of HardwareTask */
  HardwareTaskHandle = osThreadNew(StartHardwareTask, NULL, &HardwareTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* creation of gControllerEventGroup */
  gControllerEventGroupHandle = osEventFlagsNew(&gControllerEventGroup_attributes);

  /* creation of INITIALIZATION_COMPLETE */
  INITIALIZATION_COMPLETEHandle = osEventFlagsNew(&INITIALIZATION_COMPLETE_attributes);

  /* USER CODE BEGIN RTOS_EVENTS */

  LOG_INFO("main.c:\t - check if all Tasks are created correctly...");
  // check if all Tasks are created correctly
  if (AliveTaskHandle == NULL) {
	printf("main.c:\t - Error: Failed to create AliveTask\r\n");
    Error_Handler();
  }
   if (NetworkTaskHandle == NULL) {
	printf("main.c:\t - Error: Failed to create NetworkTaskHandle\r\n");
    Error_Handler();
  }

  if (WaterControllerHandle == NULL) {
	printf("main.c:\t - Error: Failed to create WaterControllerHandle\r\n");
    Error_Handler();
  }


  if (LightTaskHandle == NULL) {
	printf("main.c:\t - Error: Failed to create LightTaskHandle\r\n");
    Error_Handler();
  }

  if (SensorTaskHandle == NULL) {
	printf("main.c:\t - Error: Failed to create SensorTaskHandle\r\n");
    Error_Handler();
  }

  if (HardwareTaskHandle == NULL) {
	printf("main.c:\t - Error: Failed to create HardwareTaskHandle\r\n");
    Error_Handler();
  }

  LOG_INFO("main.c:\t check done");



  /* Überprüfen Sie, ob die Mutexe erfolgreich erstellt wurden */
  if (gControllerStateMutexHandle == NULL ||
      gGrowCycleConfigMutexHandle == NULL ||
      gEepromMutexHandle == NULL) {
      printf("main.c:\t Error: Failed to create one or more mutexes\r\n");
      Error_Handler();
  }

  LOG_INFO("main.c:\t calling InitializeGrowCycleConfig");
  InitializeGrowCycleConfig();


  printf("main.c:\t - done\r\n");
  /* add events, ... */

  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 400000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

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
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 83;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 83;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 999;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief TIM10 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM10_Init(void)
{

  /* USER CODE BEGIN TIM10_Init 0 */

  /* USER CODE END TIM10_Init 0 */

  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM10_Init 1 */

  /* USER CODE END TIM10_Init 1 */
  htim10.Instance = TIM10;
  htim10.Init.Prescaler = 84;
  htim10.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim10.Init.Period = 999;
  htim10.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim10.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim10) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim10) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim10, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM10_Init 2 */

  /* USER CODE END TIM10_Init 2 */
  HAL_TIM_MspPostInit(&htim10);

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
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CS_I2C_SPI_GPIO_Port, CS_I2C_SPI_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(OTG_FS_PowerSwitchOn_GPIO_Port, OTG_FS_PowerSwitchOn_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, W5500_ChipSelect_Pin|W5500_ChipReset_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, LD4_Pin|LD3_Pin|LD5_Pin|LD6_Pin
                          |Audio_RST_Pin|PUMP2_IN1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, PUMP2_IN2_Pin|PUMP1_IN1_Pin|PUMP1_IN2_Pin|PUMP1_ENABLE_Pin
                          |PUMP2_ENABLE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : CS_I2C_SPI_Pin */
  GPIO_InitStruct.Pin = CS_I2C_SPI_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CS_I2C_SPI_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : OTG_FS_PowerSwitchOn_Pin W5500_ChipSelect_Pin W5500_ChipReset_Pin */
  GPIO_InitStruct.Pin = OTG_FS_PowerSwitchOn_Pin|W5500_ChipSelect_Pin|W5500_ChipReset_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : BOOT1_Pin */
  GPIO_InitStruct.Pin = BOOT1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BOOT1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD4_Pin LD3_Pin LD5_Pin LD6_Pin
                           Audio_RST_Pin PUMP2_IN1_Pin */
  GPIO_InitStruct.Pin = LD4_Pin|LD3_Pin|LD5_Pin|LD6_Pin
                          |Audio_RST_Pin|PUMP2_IN1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : WATER_OBEN_Pin WATER_UNTEN_Pin */
  GPIO_InitStruct.Pin = WATER_OBEN_Pin|WATER_UNTEN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_OverCurrent_Pin */
  GPIO_InitStruct.Pin = OTG_FS_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OTG_FS_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PUMP2_IN2_Pin PUMP1_IN1_Pin PUMP1_IN2_Pin PUMP1_ENABLE_Pin
                           PUMP2_ENABLE_Pin */
  GPIO_InitStruct.Pin = PUMP2_IN2_Pin|PUMP1_IN1_Pin|PUMP1_IN2_Pin|PUMP1_ENABLE_Pin
                          |PUMP2_ENABLE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : MEMS_INT2_Pin */
  GPIO_InitStruct.Pin = MEMS_INT2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(MEMS_INT2_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

// Funktion zur Initialisierung der Heartbeat-Daten
void InitializeTaskHeartbeats(void) {
    osMutexAcquire(gHeartbeatMutexHandle, osWaitForever);
    for (int i = 0; i < MAX_MONITORED_TASKS; i++) {
        task_heartbeats[i].task_name = NULL;
        task_heartbeats[i].last_heartbeat = 0;
    }
    osMutexRelease(gHeartbeatMutexHandle);
}

// Funktion zur Registrierung einer Task
bool RegisterTaskHeartbeat(const char *task_name) {
    bool registered = false;
    osMutexAcquire(gHeartbeatMutexHandle, osWaitForever);
    for (int i = 0; i < MAX_MONITORED_TASKS; i++) {
        if (task_heartbeats[i].task_name == NULL) {
            task_heartbeats[i].task_name = task_name;
            task_heartbeats[i].last_heartbeat = HAL_GetTick();
            registered = true;
            break;
        }
    }
    osMutexRelease(gHeartbeatMutexHandle);
    return registered;
}

// Funktion zur Aktualisierung des Heartbeats einer Task
void UpdateTaskHeartbeat(const char *task_name) {
    osMutexAcquire(gHeartbeatMutexHandle, osWaitForever);
    for (int i = 0; i < MAX_MONITORED_TASKS; i++) {
        if (task_heartbeats[i].task_name != NULL && strcmp(task_heartbeats[i].task_name, task_name) == 0) {
            task_heartbeats[i].last_heartbeat = HAL_GetTick();
            break;
        }
    }
    osMutexRelease(gHeartbeatMutexHandle);
}
/* USER CODE END 4 */

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
