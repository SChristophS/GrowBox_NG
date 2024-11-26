#ifndef EEPROM_H
#define EEPROM_H

#include <stdint.h>
#include <stdbool.h>

#define EEPROM_SIZE 32768 // Größe des AT24C256 in Bytes

bool EEPROM_Write(uint16_t memAddress, uint8_t *data, uint16_t size);
bool EEPROM_Read(uint16_t memAddress, uint8_t *data, uint16_t size);
bool EEPROM_Erase(void);

#endif // EEPROM_H
