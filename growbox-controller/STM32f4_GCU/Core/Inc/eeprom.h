#ifndef EEPROM_H
#define EEPROM_H

#include <stdint.h>
#include <stdbool.h>

bool EEPROM_Write(uint16_t memAddress, uint8_t *data, uint16_t size);
bool EEPROM_Read(uint16_t memAddress, uint8_t *data, uint16_t size);
bool EEPROM_Clear(uint16_t size);

#endif // EEPROM_H
