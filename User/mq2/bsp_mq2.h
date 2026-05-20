#ifndef __BSP_MQ2_H
#define __BSP_MQ2_H

#include "stm32f4xx.h"

/* MQ2 AO = PA1 (ADC1_CH1), DO = PC5
 * ADC 配置由 bsp_mq135.c 统一管理 (双通道 DMA 扫描) */

#define MQ2_DO_PORT     GPIOC
#define MQ2_DO_PIN      GPIO_Pin_5

/* 传感器常数 */
#define MQ2_RL  10.0f
#define MQ2_R0  20.0f
#define MQ2_VC  5.0f
#define MQ2_A   43.03f
#define MQ2_B   (-1.66f)

void MQ2_Init(void);
float MQ2_Get_PPM(void);

#endif
