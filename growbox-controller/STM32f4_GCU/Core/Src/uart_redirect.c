// uart_redirect.c
#include "uart_redirect.h"
#include "stm32f4xx_hal.h" // oder die entsprechende HAL Header-Datei

extern UART_HandleTypeDef huart2;

int _write(int fd, char *str, int len) {
  for (int i = 0; i < len; i++) {
    HAL_UART_Transmit(&huart2, (uint8_t *)&str[i], 1, 0xFFFF);
  }
  return len;
}
