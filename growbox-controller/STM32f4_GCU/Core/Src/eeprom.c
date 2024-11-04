#include "eeprom.h"
#include <stdbool.h>
#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "uart_redirect.h"
#include <stdio.h>


extern I2C_HandleTypeDef hi2c2;

#define EEPROM_DEVICE_ADDRESS 0x50 << 1 // EEPROM I2C-Adresse

bool EEPROM_Init() {
    // Initialisierung des I2C-Busses, falls nötig
    return true;
}

bool EEPROM_Write(uint16_t memAddress, uint8_t *data, uint16_t size) {
    uint16_t bytesWritten = 0;
    uint16_t pageSize = 64; // Seitengröße des AT24C256
    uint16_t bytesToWrite;
    HAL_StatusTypeDef status;

    while (bytesWritten < size) {
        // Berechne, wie viele Bytes in die aktuelle Seite passen
        uint16_t pageOffset = memAddress % pageSize;
        bytesToWrite = pageSize - pageOffset; // Platz bis zum Ende der Seite

        // Schreibe nicht mehr als die verbleibende Datenmenge
        if (bytesToWrite > (size - bytesWritten)) {
            bytesToWrite = size - bytesWritten;
        }

        // Schreibe die Daten
        status = HAL_I2C_Mem_Write(&hi2c2, EEPROM_DEVICE_ADDRESS, memAddress, I2C_MEMADD_SIZE_16BIT, data + bytesWritten, bytesToWrite, 1000);
        if (status != HAL_OK) {
            printf("EEPROM_Write: Error writing at address 0x%04X\r\n", memAddress);
            return false;
        }

        // Warte auf Abschluss des Schreibvorgangs
        HAL_Delay(5);

        printf("EEPROM_Write: Writing %d bytes to address 0x%04X\r\n", size, memAddress);


        // Aktualisiere die Schreibposition
        memAddress += bytesToWrite;
        bytesWritten += bytesToWrite;
    }

    return true;
}


bool EEPROM_Read(uint16_t memAddress, uint8_t *data, uint16_t size) {
    uint16_t bytesRead = 0;
    uint16_t pageSize = 64; // Seitengröße des AT24C256
    uint16_t bytesToRead;
    HAL_StatusTypeDef status;

    while (bytesRead < size) {
        // Berechne, wie viele Bytes in die aktuelle Seite passen
        uint16_t pageOffset = memAddress % pageSize;
        bytesToRead = pageSize - pageOffset; // Platz bis zum Ende der Seite

        // Lese nicht mehr als die verbleibende Datenmenge
        if (bytesToRead > (size - bytesRead)) {
            bytesToRead = size - bytesRead;
        }

        // Lese die Daten
        status = HAL_I2C_Mem_Read(&hi2c2, EEPROM_DEVICE_ADDRESS, memAddress, I2C_MEMADD_SIZE_16BIT, data + bytesRead, bytesToRead, 1000);
        if (status != HAL_OK) {
            printf("EEPROM_Read: Error reading at address 0x%04X\r\n", memAddress);
            return false;
        }

        // Aktualisiere die Leseposition
        memAddress += bytesToRead;
        bytesRead += bytesToRead;
    }

    return true;
}

