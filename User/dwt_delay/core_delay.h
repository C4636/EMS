#ifndef __CORE_DELAY_H
#define __CORE_DELAY_H

#include "stm32f4xx.h"

#define USE_DWT_DELAY           1

#if USE_DWT_DELAY

#define GET_CPU_ClkFreq()       (SystemCoreClock)
#define SysClockFreq            (SystemCoreClock)
#define CPU_TS_INIT_IN_DELAY_FUNCTION   1

uint32_t CPU_TS_TmrRd(void);
void CPU_TS_TmrInit(void);
void CPU_TS_Tmr_Delay_US(uint32_t us);
#define CPU_TS_Tmr_Delay_MS(ms)     CPU_TS_Tmr_Delay_US(ms*1000)

#endif

#endif /* __CORE_DELAY_H */
