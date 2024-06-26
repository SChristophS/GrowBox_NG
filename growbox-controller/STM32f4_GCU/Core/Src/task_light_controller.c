#include "cmsis_os.h"
#include "controller_state.h"
#include "uart_redirect.h"
#include <stdio.h>
#include "main.h"
#include "task_light_controller.h"


void PWM_SetDutyCycle(TIM_HandleTypeDef *htim, uint32_t Channel, uint16_t DutyCycle)
{
    // Berechne die Pulse-Werte basierend auf dem Periodenwert
    uint32_t Pulse = ((htim->Init.Period + 1) * DutyCycle) / 100 - 1;

    printf("task_light_controller.c:\tset dutyciclye to: %ld\r\n", Pulse);

    __HAL_TIM_SET_COMPARE(htim, Channel, Pulse);
}


void UpdateLightControllerState(int lightIntensity) {
	printf("task_light_controller.c:\tUpdate LightIntensity to: %d\r\n", lightIntensity);

    osMutexAcquire(gControllerStateMutex, osWaitForever);

    if (gControllerState.lightIntensity != lightIntensity) {
        gControllerState.lightIntensity = lightIntensity;
        printf("task_light_controller.c: changed lightIntensity in gControllerState to %d\r\n", lightIntensity);

        osEventFlagsSet(gControllerEventGroup, LIGHT_INTESITY_CHANGED_BIT);
    }
    osMutexRelease(gControllerStateMutex);
}

void ControlLight(int lightIntensity) {
	printf("task_light_controller.c: send new PWM to controller\r\n");
	PWM_SetDutyCycle(&LED_DIM_TIM, LED_DIM_CHANNEL, lightIntensity);

}

void StartLightTask(void *argument)
{
  /* USER CODE BEGIN StartLightTask */

	int lightIntensity;
	int lightIntensity_before = 0;

  // Initialisiere den PWM-Timer
  if (HAL_TIM_PWM_Start(&LED_DIM_TIM, LED_DIM_CHANNEL) != HAL_OK) {
	  // Fehlerbehandlung, falls der Timer nicht gestartet werden kann
	  printf("task_light_controller.c: Error! Unable to start LED Timer\r\n");
	Error_Handler();
  }

  // light off as default
  PWM_SetDutyCycle(&LED_DIM_TIM, LED_DIM_CHANNEL, 0);


  /* Infinite loop */
  for(;;)
  {
      // Warte auf Nachrichten in der WaterController-Queue, mit Timeout
      if (osMessageQueueGet(xLightControllerQueueHandle, &lightIntensity, NULL, 100) == osOK) {
          // Nachricht drucken
          printf("task_light_controller.c:\tNeuer Wert lightIntensity: %d\r\n", lightIntensity);

          // Update Controller State
          UpdateLightControllerState(lightIntensity);
      }

      if (gControllerState.lightIntensity != lightIntensity_before) {
    	  lightIntensity_before = gControllerState.lightIntensity;
    	  ControlLight(gControllerState.lightIntensity);
      }

    osDelay(100);

  }
  /* USER CODE END StartLightTask */
}
