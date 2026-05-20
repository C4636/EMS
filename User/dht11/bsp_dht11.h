#ifndef __BSP_DHT11_H
#define __BSP_DHT11_H

#include "stm32f4xx.h"

#define DHT11_PORT      GPIOA
#define DHT11_PIN       GPIO_Pin_6
#define DHT11_CLK       RCC_AHB1Periph_GPIOA

void DHT11_Init(void);
uint8_t DHT11_Read_Data(uint8_t *temp, uint8_t *humi);

#endif
