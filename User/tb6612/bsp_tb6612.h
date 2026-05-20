#ifndef __BSP_TB6612_H
#define __BSP_TB6612_H

#include "stm32f4xx.h"

#define TB6612_TIM                  TIM3
#define TB6612_TIM_CLK              RCC_APB1Periph_TIM3
#define TB6612_TIM_CHANNEL          TIM_Channel_3

#define TB6612_PWMA_PORT            GPIOB
#define TB6612_PWMA_PIN             GPIO_Pin_0
#define TB6612_PWMA_SOURCE          GPIO_PinSource0
#define TB6612_PWMA_AF              GPIO_AF_TIM3
#define TB6612_PWMA_CLK             RCC_AHB1Periph_GPIOB

#define TB6612_AIN1_PORT            GPIOC
#define TB6612_AIN1_PIN             GPIO_Pin_0
#define TB6612_AIN2_PORT            GPIOC
#define TB6612_AIN2_PIN             GPIO_Pin_1
#define TB6612_STBY_PORT            GPIOC
#define TB6612_STBY_PIN             GPIO_Pin_2
#define TB6612_GPIO_CLK             RCC_AHB1Periph_GPIOC

void TB6612_Init(void);
void TB6612_SetSpeed(uint16_t duty);
void TB6612_Forward(void);
void TB6612_Backward(void);
void TB6612_Stop(void);

#endif
