/* hardware.c */

#include "hardware.h"
#include "main.h"
#include "cmsis_os.h"
#include <stdio.h>
#include "uart_redirect.h"
#include "task_hardware.h"
#include "stm32f4xx_hal.h"
//
//void EnablePump(uint8_t pumpId)
//{
//    if (pumpId == PUMP_ZULAUF) {
//    	printf("hardware.c: Enable pump with deviceID %d == PUMP_ZULAUF\r\n", pumpId);
//        HAL_GPIO_WritePin(PUMP1_ENABLE_GPIO_Port, PUMP1_ENABLE_Pin, GPIO_PIN_SET);
//
//    } else if (pumpId == PUMP_ABLAUF) {
//    	printf("hardware.c: Enable pump with deviceID %d == PUMP_ABLAUF\r\n", pumpId);
//        HAL_GPIO_WritePin(PUMP2_ENABLE_GPIO_Port, PUMP2_ENABLE_Pin, GPIO_PIN_SET);
//        // Weitere Pins setzen, falls nötig
//    }
//}
//
//void DisablePump(uint8_t pumpId)
//{
//    if (pumpId == PUMP_ZULAUF) {
//    	printf("hardware.c: Disable pump with deviceID %d == PUMP_ZULAUF\r\n", pumpId);
//        HAL_GPIO_WritePin(PUMP2_ENABLE_GPIO_Port, PUMP2_ENABLE_Pin, GPIO_PIN_RESET);
//        // Weitere Pins zurücksetzen, falls nötig
//    } else if (pumpId == PUMP_ABLAUF) {
//    	printf("hardware.c: Disable pump with deviceID %d == PUMP_ZULAUF\r\n", pumpId);
//    	HAL_GPIO_WritePin(PUMP1_ENABLE_GPIO_Port, PUMP1_ENABLE_Pin, GPIO_PIN_RESET);
//
//        // Weitere Pins zurücksetzen, falls nötig
//    }
//}

void EnablePump(uint8_t pumpId)
{
    if (pumpId == PUMP_ABLAUF) {
        printf("hardware.c: Enable pump with deviceID %d == PUMP_ZULAUF\r\n", pumpId);
        // Setze Richtungspins
        HAL_GPIO_WritePin(PUMP1_IN1_GPIO_Port, PUMP1_IN1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(PUMP1_IN2_GPIO_Port, PUMP1_IN2_Pin, GPIO_PIN_RESET);
        // Aktiviere die Pumpe
        HAL_GPIO_WritePin(PUMP1_ENABLE_GPIO_Port, PUMP1_ENABLE_Pin, GPIO_PIN_SET);
    } else if (pumpId == PUMP_ZULAUF) {
        printf("hardware.c: Enable pump with deviceID %d == PUMP_ABLAUF\r\n", pumpId);
        // Setze Richtungspins
        HAL_GPIO_WritePin(PUMP2_IN1_GPIO_Port, PUMP2_IN1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(PUMP2_IN2_GPIO_Port, PUMP2_IN2_Pin, GPIO_PIN_RESET);
        // Aktiviere die Pumpe
        HAL_GPIO_WritePin(PUMP2_ENABLE_GPIO_Port, PUMP2_ENABLE_Pin, GPIO_PIN_SET);
    }
}

void DisablePump(uint8_t pumpId)
{
    if (pumpId == PUMP_ABLAUF) {
        printf("hardware.c: Disable pump with deviceID %d == PUMP_ZULAUF\r\n", pumpId);
        // Deaktiviere die Pumpe
        HAL_GPIO_WritePin(PUMP1_ENABLE_GPIO_Port, PUMP1_ENABLE_Pin, GPIO_PIN_RESET);
        // Setze Richtungspins zurück
        HAL_GPIO_WritePin(PUMP1_IN1_GPIO_Port, PUMP1_IN1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(PUMP1_IN2_GPIO_Port, PUMP1_IN2_Pin, GPIO_PIN_RESET);
    } else if (pumpId == PUMP_ZULAUF) {
        printf("hardware.c: Disable pump with deviceID %d == PUMP_ABLAUF\r\n", pumpId);
        // Deaktiviere die Pumpe
        HAL_GPIO_WritePin(PUMP2_ENABLE_GPIO_Port, PUMP2_ENABLE_Pin, GPIO_PIN_RESET);
        // Setze Richtungspins zurück
        HAL_GPIO_WritePin(PUMP2_IN1_GPIO_Port, PUMP2_IN1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(PUMP2_IN2_GPIO_Port, PUMP2_IN2_Pin, GPIO_PIN_RESET);
    }
}


void PWM_SetDutyCycle(TIM_HandleTypeDef *htim, uint32_t Channel, uint16_t DutyCycle)
{
    // Invertiere den DutyCycle-Wert
    uint32_t InvertedDutyCycle = 100 - DutyCycle;

    // Berechne den Pulse-Wert basierend auf der Periodenwert
    uint32_t Pulse = (InvertedDutyCycle * (htim->Init.Period + 1)) / 100;

    printf("task_light_controller.c:\tset dutyciclye to: %lu\r\n", Pulse);

    __HAL_TIM_SET_COMPARE(htim, Channel, Pulse);
}

void SetLightIntensity(uint8_t intensity)
{
    // Implementiere die PWM-Steuerung für die Lichtintensität
	PWM_SetDutyCycle(&LED_DIM_TIM, LED_DIM_CHANNEL, intensity);
}


bool ReadSensorOben(void) {
    return HAL_GPIO_ReadPin(WATER_OBEN_GPIO_Port, WATER_OBEN_Pin) == GPIO_PIN_SET;
}

bool ReadSensorUnten(void) {
    return HAL_GPIO_ReadPin(WATER_UNTEN_GPIO_Port, WATER_UNTEN_Pin) == GPIO_PIN_SET;
}
