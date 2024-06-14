#ifndef __WIZCHIP_INIT_H__
#define __WIZCHIP_INIT_H__


#include "main.h"
#include "wizchip_conf.h"
#include "stm32f4xx_hal.h"

/* SPI Handle */
extern SPI_HandleTypeDef hspi2;
#define WIZCHIP_SPI  hspi2

/* Chip Select (CS) Pin */
#define WIZCHIP_CS_PIN    W5500_ChipSelect_Pin
#define WIZCHIP_CS_PORT   W5500_ChipSelect_GPIO_Port

/* Chip Reset (RESET) Pin */
#define W5x00_RESET_PIN   W5500_ChipReset_Pin
#define W5x00_RESET_PORT  W5500_ChipReset_GPIO_Port

/* Function Prototypes */
void WIZCHIPInitialize();
void resetAssert(void);
void resetDeassert(void);



//#define W5x00_RESET_PIN     W5x00_RESET_Pin
//#define W5x00_RESET_PORT    W5x00_RESET_GPIO_Port

void csEnable(void);
void csDisable(void);
void spiWriteByte(uint8_t tx);
uint8_t spiReadByte(void);

#endif
