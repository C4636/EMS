#ifndef __BSP_KEY_H
#define __BSP_KEY_H

#include "stm32f4xx.h"

#define KEY1_PIN        GPIO_Pin_0
#define KEY1_PORT       GPIOA
#define KEY1_CLK        RCC_AHB1Periph_GPIOA

#define KEY2_PIN        GPIO_Pin_13
#define KEY2_PORT       GPIOC
#define KEY2_CLK        RCC_AHB1Periph_GPIOC

#define KEY_ON  1
#define KEY_OFF 0

void Key_Init(void);
uint8_t Key_Scan(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

#endif
