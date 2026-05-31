#ifndef __BSP_DWT_DELAY_H
#define __BSP_DWT_DELAY_H

#include "./dwt_delay/core_delay.h"

/* compatibility wrappers for existing code */
#define DWT_Delay_Init()    CPU_TS_TmrInit()
#define DWT_Delay_us(us)    CPU_TS_Tmr_Delay_US(us)
#define DWT_Delay_ms(ms)    CPU_TS_Tmr_Delay_MS(ms)

#endif
