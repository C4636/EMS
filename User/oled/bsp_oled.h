#ifndef __BSP_OLED_H
#define __BSP_OLED_H

#include "stm32f4xx.h"

/* I2C1 引脚: PB6(SCL) + PB7(SDA), 可改 PB8/PB9 */
#define OLED_I2C                I2C1
#define OLED_I2C_CLK            RCC_APB1Periph_I2C1
#define OLED_I2C_SPEED          400000

#define OLED_SCL_PORT           GPIOB
#define OLED_SCL_PIN            GPIO_Pin_6
#define OLED_SCL_SOURCE         GPIO_PinSource6
#define OLED_SCL_AF             GPIO_AF_I2C1

#define OLED_SDA_PORT           GPIOB
#define OLED_SDA_PIN            GPIO_Pin_7
#define OLED_SDA_SOURCE         GPIO_PinSource7
#define OLED_SDA_AF             GPIO_AF_I2C1

#define OLED_I2C_GPIO_CLK       RCC_AHB1Periph_GPIOB

/* SSD1306 7-bit 地址 (DC引脚接地=0x3C, 接VCC=0x3D) */
#define OLED_ADDR               0x3C

void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowString(uint8_t x, uint8_t y, const char *str);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len);
void OLED_Refresh(void);

#endif
