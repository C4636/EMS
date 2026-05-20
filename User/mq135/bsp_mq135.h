#ifndef __BSP_MQ135_H
#define __BSP_MQ135_H

#include "stm32f4xx.h"

/* MQ135 AO = PA4 (ADC1_CH4), DO = PC4 */
#define MQ135_ADC_GPIO_PORT     GPIOA
#define MQ135_ADC_GPIO_PIN      GPIO_Pin_4
#define MQ135_ADC_GPIO_CLK      RCC_AHB1Periph_GPIOA
#define MQ135_ADC_CHANNEL       ADC_Channel_4

#define MQ135_DO_PORT           GPIOC
#define MQ135_DO_PIN            GPIO_Pin_4
#define MQ135_DO_CLK            RCC_AHB1Periph_GPIOC

/* 传感器常数 */
#define MQ135_RL  1.0f
#define MQ135_R0  2.0f
#define MQ135_VC  5.0f
#define MQ135_A   4.17f
#define MQ135_B   (-2.28f)

/* ADC1 共用 (MQ135 + MQ2 双通道 DMA 扫描) */
#define MQ_ADC                  ADC1
#define MQ_ADC_CLK              RCC_APB2Periph_ADC1
#define MQ_ADC_CHANNEL_COUNT    2
#define MQ_ADC_DR_ADDR          ((uint32_t)ADC1 + 0x4C)

#define MQ_ADC_DMA              DMA2_Stream0
#define MQ_ADC_DMA_CLK          RCC_AHB1Periph_DMA2
#define MQ_ADC_DMA_CHANNEL      DMA_Channel_0

extern __IO uint16_t MQ_ADC_Values[MQ_ADC_CHANNEL_COUNT];
// MQ_ADC_Values[0] = MQ2 (CH1), MQ_ADC_Values[1] = MQ135 (CH4)

void MQ135_Init(void);
float MQ135_Get_PPM(void);

#endif
