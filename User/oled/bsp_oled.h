#ifndef __BSP_OLED_H
#define __BSP_OLED_H

#include "stm32f4xx.h"

#define OLED_ID         0x78
#define OLED_WR_CMD     0x00
#define OLED_WR_DATA    0x40

void OLED_Init(void);
void OLED_SetPos(uint8_t x, uint8_t y);
void OLED_Fill(uint8_t fill_data);
void OLED_CLS(void);
void OLED_ON(void);
void OLED_OFF(void);
void OLED_ShowStr(uint8_t x, uint8_t y, const char *ch, uint8_t text_size);
void OLED_ShowCN(uint8_t x, uint8_t y, uint8_t n);
void OLED_DrawBMP(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, const uint8_t BMP[]);
void OLED_Refresh(void);

#endif
