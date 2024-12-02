/* hardware.c */

#include "hardware.h"
#include "main.h"
#include "cmsis_os.h"
#include <stdio.h>
#include "uart_redirect.h"
#include "task_hardware.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_def.h"
#include "logger.h"

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

HAL_StatusTypeDef PWM_SetDutyCycle(TIM_HandleTypeDef *htim, uint32_t Channel, uint16_t DutyCycle)
{
    // Überprüfe, ob der DutyCycle-Wert gültig ist (0 bis 100)
    if (DutyCycle > 100) {
        LOG_WARN("PWM_SetDutyCycle: Invalid DutyCycle value %d. Value should be between 0 and 100.", DutyCycle);
        return HAL_ERROR;
    }

    // Invertiere den DutyCycle-Wert
    uint32_t InvertedDutyCycle = 100 - DutyCycle;

    // Berechne den Pulse-Wert basierend auf dem Periodenwert
    uint32_t Pulse = (InvertedDutyCycle * (htim->Init.Period + 1)) / 100;

    LOG_INFO("PWM_SetDutyCycle: Setting duty cycle to: %lu", Pulse);

    // Überprüfe, ob der Timer korrekt initialisiert wurde
    if (htim == NULL) {
        LOG_ERROR("PWM_SetDutyCycle: Timer handle is NULL");
        return HAL_ERROR;
    }

    // Setze den neuen Compare-Wert
    __HAL_TIM_SET_COMPARE(htim, Channel, Pulse);

    return HAL_OK;
}



HAL_StatusTypeDef SetLightIntensity(uint8_t intensity)
{
    // Überprüfe, ob die Intensität innerhalb des gültigen Bereichs liegt
    if (intensity > 100) {
        LOG_WARN("SetLightIntensity: Invalid intensity value %d. Value should be between 0 and 100.", intensity);
        return HAL_ERROR;
    }

    // Implementiere die PWM-Steuerung für die Lichtintensität
    HAL_StatusTypeDef status = PWM_SetDutyCycle(&LED_DIM_TIM, LED_DIM_CHANNEL, intensity);
    if (status != HAL_OK) {
        LOG_ERROR("SetLightIntensity: Failed to set light intensity to %d", intensity);
    } else {
        LOG_INFO("SetLightIntensity: Light intensity set to %d", intensity);
    }

    return status;
}




bool ReadSensorOben(void) {
    return HAL_GPIO_ReadPin(WATER_OBEN_GPIO_Port, WATER_OBEN_Pin) == GPIO_PIN_SET;
}

bool ReadSensorUnten(void) {
    return HAL_GPIO_ReadPin(WATER_UNTEN_GPIO_Port, WATER_UNTEN_Pin) == GPIO_PIN_SET;
}
