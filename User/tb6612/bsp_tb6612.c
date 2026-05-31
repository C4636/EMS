/**
 * TB6612FNG motor driver - PWM speed + direction control
 * PWMA=PB0(TIM3_CH3), AIN1=PC0, AIN2=PC1, STBY=PC2
 */
#include "./tb6612/bsp_tb6612.h"

void TB6612_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    /* GPIO clocks */
    RCC_AHB1PeriphClockCmd(TB6612_PWMA_CLK, ENABLE);
    RCC_AHB1PeriphClockCmd(TB6612_GPIO_CLK, ENABLE);

    /* PWMA = AF push-pull for TIM3_CH3 */
    GPIO_InitStructure.GPIO_Pin   = TB6612_PWMA_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(TB6612_PWMA_PORT, &GPIO_InitStructure);
    GPIO_PinAFConfig(TB6612_PWMA_PORT, TB6612_PWMA_SOURCE, TB6612_PWMA_AF);

    /* AIN1, AIN2, STBY = push-pull output */
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin   = TB6612_AIN1_PIN | TB6612_AIN2_PIN | TB6612_STBY_PIN;
    GPIO_Init(TB6612_AIN1_PORT, &GPIO_InitStructure);

    /* STBY high = always enabled */
    GPIO_SetBits(TB6612_STBY_PORT, TB6612_STBY_PIN);

    /* TIM3 PWM: 84MHz / (84 * 100) = 10kHz */
    RCC_APB1PeriphClockCmd(TB6612_TIM_CLK, ENABLE);

    TIM_TimeBaseStructure.TIM_Prescaler         = TB6612_TIM_PSC - 1;
    TIM_TimeBaseStructure.TIM_Period            = TB6612_TIM_ARR - 1;
    TIM_TimeBaseStructure.TIM_CounterMode       = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_ClockDivision     = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TB6612_TIM, &TIM_TimeBaseStructure);

    /* PWM1 mode, active high */
    TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse       = 0;  /* start at 0% */
    TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
    TIM_OC3Init(TB6612_TIM, &TIM_OCInitStructure);

    TIM_OC3PreloadConfig(TB6612_TIM, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TB6612_TIM, ENABLE);
    TIM_Cmd(TB6612_TIM, ENABLE);
    TIM_CtrlPWMOutputs(TB6612_TIM, ENABLE);

    /* Stop motor initially */
    TB6612_Stop();
}

void TB6612_SetSpeed(uint16_t duty)
{
    if (duty > 100) duty = 100;
    TB6612_TIM_CCRX = duty;  /* TIM3->CCR3 = duty (0-100) */
}

void TB6612_Forward(void)
{
    GPIO_ResetBits(TB6612_AIN1_PORT, TB6612_AIN1_PIN);  /* AIN1=0 */
    GPIO_SetBits(TB6612_AIN2_PORT, TB6612_AIN2_PIN);    /* AIN2=1 */
}

void TB6612_Backward(void)
{
    GPIO_SetBits(TB6612_AIN1_PORT, TB6612_AIN1_PIN);    /* AIN1=1 */
    GPIO_ResetBits(TB6612_AIN2_PORT, TB6612_AIN2_PIN);  /* AIN2=0 */
}

void TB6612_Stop(void)
{
    GPIO_ResetBits(TB6612_AIN1_PORT, TB6612_AIN1_PIN);  /* short brake */
    GPIO_ResetBits(TB6612_AIN2_PORT, TB6612_AIN2_PIN);
}
