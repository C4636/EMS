/**
 * MQ2 烟雾传感器
 * GPIO 初始化 + PPM 计算
 * ADC 由 bsp_mq135.c 统一配置 (ADC1 双通道 DMA)
 */
#include "./mq2/bsp_mq2.h"
#include "./mq135/bsp_mq135.h"
#include "math.h"

void MQ2_Init(void)
{
    // GPIO 已在 MQ135_Init() → MQ_ADC_GPIO_Config() 中配置
    // ADC 已在 MQ135_Init() → MQ_ADC_Mode_Config() 中配置
}

float MQ2_Get_PPM(void)
{
    float vrl = (float)MQ_ADC_Values[0] / 4096.0f * MQ2_VC;
    float Rs  = (MQ2_VC - vrl) * MQ2_RL / vrl;
    return MQ2_A * powf(Rs / MQ2_R0, MQ2_B);
}
