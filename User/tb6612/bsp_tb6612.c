/**
 * TB6612FNG 电机驱动 — 待实现
 * PWMA -> PB0 (TIM3_CH3), AIN1 -> PC0, AIN2 -> PC1, STBY -> PC2
 */
#include "./tb6612/bsp_tb6612.h"

void TB6612_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(TB6612_PWMA_CLK, ENABLE);
    RCC_AHB1PeriphClockCmd(TB6612_GPIO_CLK, ENABLE);

    // PWMA — 复用推挽 (PWM)
    GPIO_InitStructure.GPIO_Pin = TB6612_PWMA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(TB6612_PWMA_PORT, &GPIO_InitStructure);
    GPIO_PinAFConfig(TB6612_PWMA_PORT, TB6612_PWMA_SOURCE, TB6612_PWMA_AF);

    // AIN1, AIN2, STBY — 推挽输出
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Pin = TB6612_AIN1_PIN | TB6612_AIN2_PIN | TB6612_STBY_PIN;
    GPIO_Init(TB6612_AIN1_PORT, &GPIO_InitStructure);

    // STBY 拉高使能
    GPIO_SetBits(TB6612_STBY_PORT, TB6612_STBY_PIN);
    // 停止状态
    GPIO_ResetBits(TB6612_AIN1_PORT, TB6612_AIN1_PIN | TB6612_AIN2_PIN);

    // TODO: 配置 TIM3_CH3 PWM 输出 (10kHz, 初始占空比 0%)
}

void TB6612_SetSpeed(uint16_t duty) { (void)duty; /* TODO: TIM3->CCR3 = duty */ }
void TB6612_Forward(void) { /* TODO: AIN1=0, AIN2=1 */ }
void TB6612_Backward(void) { /* TODO: AIN1=1, AIN2=0 */ }
void TB6612_Stop(void) { /* TODO: AIN1=0, AIN2=0 */ }
