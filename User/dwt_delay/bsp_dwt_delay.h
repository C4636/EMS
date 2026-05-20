#ifndef __BSP_DWT_DELAY_H
#define __BSP_DWT_DELAY_H

#include "stm32f4xx.h"

void DWT_Delay_Init(void);
void DWT_Delay_us(uint32_t us);
void DWT_Delay_ms(uint32_t ms);

#endif
