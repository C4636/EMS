#include "./dwt_delay/bsp_dwt_delay.h"

static uint32_t cpu_clk_mhz;

void DWT_Delay_Init(void)
{
    // 使能 DWT 外设
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    // 获取 CPU 主频 (MHz)
    SystemCoreClockUpdate();
    cpu_clk_mhz = SystemCoreClock / 1000000;
}

void DWT_Delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * cpu_clk_mhz;
    while ((DWT->CYCCNT - start) < ticks);
}

void DWT_Delay_ms(uint32_t ms)
{
    DWT_Delay_us(ms * 1000);
}
