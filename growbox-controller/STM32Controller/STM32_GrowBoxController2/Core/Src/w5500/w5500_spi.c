#include "stm32f1xx_hal.h"
#include "wizchip_conf.h"
#include "stdio.h"

// Global variable
// SPI_HandleTypeDef: This is a data type specific to the HAL (Hardware Abstraction Layer) library for STM32.
// It is used to define a structure for managing SPI (Serial Peripheral Interface) communication channels.
// This structure contains all necessary information to configure and operate an SPI channel.
extern SPI_HandleTypeDef hspi2;

// Function to read and write data over SPI
uint8_t SPIReadWrite(uint8_t data)
{
    // Wait until the Transmit Buffer is empty
    while((hspi2.Instance->SR & SPI_FLAG_TXE) != SPI_FLAG_TXE);

    // Send data over SPI
    *(__IO uint8_t*)&hspi2.Instance->DR = data;

    // Wait until the Receive Buffer is not empty
    while((hspi2.Instance->SR & SPI_FLAG_RXNE) != SPI_FLAG_RXNE);

    // Return the received data
    return (*(__IO uint8_t*)&hspi2.Instance->DR);
}


// Function to select the WIZchip
void wizchip_select(void)
{
    // The pin is configured in .ioc file and is PB1
    // Set the GPIO_PIN_RESET to make the pin low
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
}

// Function to deselect the WIZchip
void wizchip_deselect(void)
{
    // The pin is configured in .ioc file and is PB1
    // Set the GPIO_PIN_SET to make the pin high
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
}

// Function to read a single byte from WIZchip
uint8_t wizchip_read()
{
    uint8_t rb;
    // Send dummy data to read a byte
    rb = SPIReadWrite(0x00);
    return rb;
}

// Function to write a single byte to WIZchip
void wizchip_write(uint8_t wb)
{
    // Write data over SPI
    SPIReadWrite(wb);
}

// Function to read multiple bytes from WIZchip using burst mode
void wizchip_readburst(uint8_t* pBuf, uint16_t len)
{
    for(uint16_t i = 0; i < len; i++)
    {
        // Read byte and increment buffer pointer
        *pBuf = SPIReadWrite(0x00);
        pBuf++;
    }
}

// Function to write multiple bytes to WIZchip using burst mode
void wizchip_writeburst(uint8_t* pBuf, uint16_t len)
{
    for(uint16_t i = 0; i < len; i++)
    {
        // Write byte and increment buffer pointer
        SPIReadWrite(*pBuf);
        pBuf++;
    }
}

void W5500IOInit(){
    /*
     * Initialize the two GPIO pins
     * RESET -> PB2
     * and
     * CS -> PB1
     */
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Enable the clock for GPIOB
    __HAL_RCC_GPIOB_CLK_ENABLE();  // Corrected function name

    // Configure GPIO pins: PB1 for CS and PB2 for RESET
    GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2; // Corrected pin numbers
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; // Set mode to output push-pull
    GPIO_InitStruct.Pull = GPIO_NOPULL;         // No pull-up or pull-down resistors
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; // High frequency for the GPIO pins

    // Initialize the GPIO pins with the configuration
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}


void W5500Init(){
    uint8_t memsize[2][8] = { { 2, 2, 2, 2, 2, 2, 2, 2}, { 2, 2, 2, 2, 2, 2, 2, 2} };

    // Initialize the W5500 GPIO settings
    W5500IOInit();

    // Set Chip Select (CS) pin high by default
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);

    // Send a reset pulse to the W5500
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);  // Set the Reset pin low
    HAL_Delay(10);                                         // Wait for 10ms
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);    // Set the Reset pin high
    HAL_Delay(10);                                         // Wait for 10ms to stabilize

    // Register callback functions for WIZchip CS and SPI operations
    reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);
    reg_wizchip_spi_cbfunc(wizchip_read, wizchip_write);
    reg_wizchip_spiburst_cbfunc(wizchip_readburst, wizchip_writeburst);

    // Initialize WIZchip with the specified memory size configuration
    if (ctlwizchip(CW_INIT_WIZCHIP, (void*) memsize) == -1){
        printf("WIZCHIP Initialized fail.\r\n");
        while(1);  // Infinite loop to halt further execution on failure
    }
    printf("WIZCHIP Initialized success.\r\n");
}



/* original code from tutorial
void W5500Init(){
    uint8_t tmp;
    uint8_t memsize[2][8] = { { 2, 2, 2, 2, 2, 2, 2, 2}, { 2, 2, 2, 2, 2, 2, 2, 2} };

    // Initialize the W5500 GPIO settings
    W5500IOInit();

    // Set Chip Select (CS) pin high by default
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);

    // Send a reset pulse to the W5500
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
    tmp = 0xFF;
    while(tmp--);  // Simple delay loop
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);

    // Register callback functions for WIZchip CS and SPI operations
    reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);
    reg_wizchip_spi_cbfunc(wizchip_read, wizchip_write);
    reg_wizchip_spiburst_cbfunc(wizchip_readburst, wizchip_writeburst);

    // Initialize WIZchip with the specified memory size configuration
    if (ctlwizchip(CW_INIT_WIZCHIP, (void*) memsize) == -1){
        printf("WIZCHIP Initialized fail.\r\n");
        while(1);  // Infinite loop to halt further execution on failure
    }
    printf("WIZCHIP Initialized success.\r\n");
}

 */
