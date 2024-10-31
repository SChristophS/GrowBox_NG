#include "eeprom.h"
#include <stdbool.h>
#include <stdint.h>
#include "stm32f4xx_hal.h"




extern I2C_HandleTypeDef hi2c2;

#define EEPROM_DEVICE_ADDRESS 0x50 << 1 // EEPROM I2C-Adresse

bool EEPROM_Init() {
    // Initialisierung des I2C-Busses, falls nötig
    return true;
}

bool EEPROM_Write(uint16_t memAddress, uint8_t *data, uint16_t size) {
    // Aufteilen in Seiten, falls nötig
    if (HAL_I2C_Mem_Write(&hi2c2, EEPROM_DEVICE_ADDRESS, memAddress, I2C_MEMADD_SIZE_16BIT, data, size, 1000) != HAL_OK) {
        return false;
    }
    HAL_Delay(5); // Wartezeit für den Schreibvorgang
    return true;
}

bool EEPROM_Read(uint16_t memAddress, uint8_t *data, uint16_t size) {
    if (HAL_I2C_Mem_Read(&hi2c2, EEPROM_DEVICE_ADDRESS, memAddress, I2C_MEMADD_SIZE_16BIT, data, size, 1000) != HAL_OK) {
        return false;
    }
    return true;
}

bool EEPROM_Clear(uint16_t size) {
    uint8_t emptyData[64]; // Puffer für zu schreibende Daten
    memset(emptyData, 0xFF, sizeof(emptyData)); // Füllt den Puffer mit 0xFF

    uint16_t memAddress = 0;
    while (size > 0) {
        uint16_t chunkSize = (size > sizeof(emptyData)) ? sizeof(emptyData) : size;

        if (HAL_I2C_Mem_Write(&hi2c2, EEPROM_DEVICE_ADDRESS, memAddress, I2C_MEMADD_SIZE_16BIT, emptyData, chunkSize, 1000) != HAL_OK) {
            return false;
        }
        HAL_Delay(5); // Schreibverzögerung

        memAddress += chunkSize;
        size -= chunkSize;
    }
    return true;
}
