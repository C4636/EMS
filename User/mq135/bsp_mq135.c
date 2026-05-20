/**
 * MQ135 + MQ2 ADC1 双通道 DMA 模式
 * 基于野火 F407 ADC 多通道 DMA 例程
 *
 * ADC1 通道扫描顺序:
 *   [0] = CH1  (PA1) → MQ2
 *   [1] = CH4  (PA4) → MQ135
 */
#include "./mq135/bsp_mq135.h"
#include "math.h"

__IO uint16_t MQ_ADC_Values[MQ_ADC_CHANNEL_COUNT];

static void MQ_ADC_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // MQ135 AO = PA4, MQ2 AO = PA1 (同 GPIOA)
    RCC_AHB1PeriphClockCmd(MQ135_ADC_GPIO_CLK, ENABLE);
    // MQ135 DO = PC4, MQ2 DO = PC5 (同 GPIOC)
    RCC_AHB1PeriphClockCmd(MQ135_DO_CLK, ENABLE);

    // MQ135 AO — 模拟输入
    GPIO_InitStructure.GPIO_Pin   = MQ135_ADC_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(MQ135_ADC_GPIO_PORT, &GPIO_InitStructure);

    // MQ2 AO — 模拟输入 (PA1)
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_1;
    GPIO_Init(MQ135_ADC_GPIO_PORT, &GPIO_InitStructure);

    // MQ135 DO — 数字输入
    GPIO_InitStructure.GPIO_Pin   = MQ135_DO_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(MQ135_DO_PORT, &GPIO_InitStructure);

    // MQ2 DO — 数字输入 (PC5)
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_5;
    GPIO_Init(MQ135_DO_PORT, &GPIO_InitStructure);
}

static void MQ_ADC_DMA_Config(void)
{
    DMA_InitTypeDef DMA_InitStructure;

    RCC_AHB1PeriphClockCmd(MQ_ADC_DMA_CLK, ENABLE);

    DMA_InitStructure.DMA_PeripheralBaseAddr = MQ_ADC_DR_ADDR;
    DMA_InitStructure.DMA_Memory0BaseAddr    = (uint32_t)MQ_ADC_Values;
    DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralToMemory;
    DMA_InitStructure.DMA_BufferSize         = MQ_ADC_CHANNEL_COUNT;
    DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode               = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority           = DMA_Priority_High;
    DMA_InitStructure.DMA_FIFOMode           = DMA_FIFOMode_Disable;
    DMA_InitStructure.DMA_Channel            = MQ_ADC_DMA_CHANNEL;
    DMA_Init(MQ_ADC_DMA, &DMA_InitStructure);
    DMA_Cmd(MQ_ADC_DMA, ENABLE);
}

static void MQ_ADC_Mode_Config(void)
{
    ADC_CommonInitTypeDef ADC_CommonInitStructure;
    ADC_InitTypeDef       ADC_InitStructure;

    RCC_APB2PeriphClockCmd(MQ_ADC_CLK, ENABLE);

    // ADC 公共配置
    ADC_CommonInitStructure.ADC_Mode            = ADC_Mode_Independent;
    ADC_CommonInitStructure.ADC_Prescaler       = ADC_Prescaler_Div4;
    ADC_CommonInitStructure.ADC_DMAAccessMode   = ADC_DMAAccessMode_Disabled;
    ADC_CommonInitStructure.ADC_TwoSamplingDelay= ADC_TwoSamplingDelay_20Cycles;
    ADC_CommonInit(&ADC_CommonInitStructure);

    // ADC 独立配置
    ADC_InitStructure.ADC_Resolution            = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ScanConvMode          = ENABLE;          // 多通道扫描
    ADC_InitStructure.ADC_ContinuousConvMode    = ENABLE;
    ADC_InitStructure.ADC_ExternalTrigConvEdge  = ADC_ExternalTrigConvEdge_None;
    ADC_InitStructure.ADC_DataAlign             = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfConversion       = MQ_ADC_CHANNEL_COUNT;
    ADC_Init(MQ_ADC, &ADC_InitStructure);

    // 通道序列: [0]=CH1(MQ2), [1]=CH4(MQ135)
    ADC_RegularChannelConfig(MQ_ADC, ADC_Channel_1, 1, ADC_SampleTime_56Cycles);
    ADC_RegularChannelConfig(MQ_ADC, ADC_Channel_4, 2, ADC_SampleTime_56Cycles);

    ADC_DMARequestAfterLastTransferCmd(MQ_ADC, ENABLE);
    ADC_DMACmd(MQ_ADC, ENABLE);
    ADC_Cmd(MQ_ADC, ENABLE);

    ADC_SoftwareStartConv(MQ_ADC);
}

void MQ135_Init(void)
{
    MQ_ADC_GPIO_Config();
    MQ_ADC_DMA_Config();
    MQ_ADC_Mode_Config();
}

float MQ135_Get_PPM(void)
{
    float vrl = (float)MQ_ADC_Values[1] / 4096.0f * MQ135_VC;
    float Rs  = (MQ135_VC - vrl) * MQ135_RL / vrl;
    return MQ135_A * powf(Rs / MQ135_R0, MQ135_B);
}
