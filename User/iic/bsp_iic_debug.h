#ifndef __BSP_IIC_DEBUG_H
#define __BSP_IIC_DEBUG_H

#include "stm32f4xx.h"

#define IIC_SELECT  1   /* 1=hardware I2C, 0=software I2C */

/* I2C1, PB6=SCL, PB7=SDA */
#define IIC_NUM             I2C1
#define IIC_GPIO_CLK        RCC_AHB1Periph_GPIOB
#define IIC_CLK             RCC_APB1Periph_I2C1
#define IIC_PORT            GPIOB
#define IIC_SCL_PIN         GPIO_Pin_6
#define IIC_SDA_PIN         GPIO_Pin_7

/* software I2C pin macros */
#define IIC_SDA_0           GPIO_ResetBits(IIC_PORT, IIC_SDA_PIN)
#define IIC_SDA_1           GPIO_SetBits(IIC_PORT, IIC_SDA_PIN)
#define IIC_SCL_0           GPIO_ResetBits(IIC_PORT, IIC_SCL_PIN)
#define IIC_SCL_1           GPIO_SetBits(IIC_PORT, IIC_SCL_PIN)
#define IIC_SDA_READ        GPIO_ReadInputDataBit(IIC_PORT, IIC_SDA_PIN)

void IIC_GPIO_Config(void);
void IIC_Start(void);
void IIC_Stop(void);
void IIC_SendByte(uint8_t byte);
uint8_t IIC_ReciveByte(void);
void IIC_ACK(uint8_t ack);
uint8_t IIC_Wait_ACK(void);

#endif
