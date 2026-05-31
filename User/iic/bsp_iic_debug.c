/**
 * I2C driver — ported from Wildfire F407 OLED example
 * I2C1: PB6=SCL, PB7=SDA (adapted from original PB8/PB9)
 */
#include "./iic/bsp_iic_debug.h"

static void IIC_Delay(void)
{
    volatile uint32_t i;
    for (i = 0; i < 40; i++);
}

void IIC_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

#if (IIC_SELECT == 1)
    /* hardware I2C */
    RCC_AHB1PeriphClockCmd(IIC_GPIO_CLK, ENABLE);
    RCC_APB1PeriphClockCmd(IIC_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = IIC_SCL_PIN | IIC_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(IIC_PORT, &GPIO_InitStructure);

    GPIO_PinAFConfig(IIC_PORT, GPIO_PinSource6, GPIO_AF_I2C1);
    GPIO_PinAFConfig(IIC_PORT, GPIO_PinSource7, GPIO_AF_I2C1);

    I2C_InitTypeDef I2C_InitStructure;
    I2C_InitStructure.I2C_Ack                  = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress  = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed           = 400000;
    I2C_InitStructure.I2C_DutyCycle            = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_Mode                 = I2C_Mode_I2C;
    I2C_InitStructure.I2C_OwnAddress1          = 0x78;
    I2C_Init(IIC_NUM, &I2C_InitStructure);
    I2C_Cmd(IIC_NUM, ENABLE);
#else
    /* software I2C */
    RCC_AHB1PeriphClockCmd(IIC_GPIO_CLK, ENABLE);
    GPIO_InitStructure.GPIO_Pin   = IIC_SCL_PIN | IIC_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(IIC_PORT, &GPIO_InitStructure);
    IIC_SDA_1; IIC_SCL_1;
#endif
}

void IIC_Start(void)
{
    IIC_SDA_1; IIC_SCL_1; IIC_Delay();
    IIC_SDA_0; IIC_Delay();
    IIC_SCL_0; IIC_Delay();
}

void IIC_Stop(void)
{
    IIC_SDA_0; IIC_SCL_0; IIC_Delay();
    IIC_SCL_1; IIC_Delay();
    IIC_SDA_1; IIC_Delay();
}

void IIC_SendByte(uint8_t byte)
{
    uint8_t i;
    for (i = 0; i < 8; i++) {
        if (byte & 0x80) IIC_SDA_1; else IIC_SDA_0;
        byte <<= 1;
        IIC_SCL_1; IIC_Delay();
        IIC_SCL_0; IIC_Delay();
    }
    IIC_SDA_1;
    IIC_SCL_1; IIC_Delay();
    IIC_SCL_0; IIC_Delay();
}

uint8_t IIC_ReciveByte(void)
{
    uint8_t i, temp = 0;
    IIC_SDA_1;
    for (i = 0; i < 8; i++) {
        temp <<= 1;
        IIC_SCL_1; IIC_Delay();
        if (IIC_SDA_READ) temp |= 0x01;
        IIC_SCL_0; IIC_Delay();
    }
    return temp;
}

void IIC_ACK(uint8_t ack)
{
    if (ack) IIC_SDA_1; else IIC_SDA_0;
    IIC_SCL_1; IIC_Delay();
    IIC_SCL_0; IIC_Delay();
    IIC_SDA_1;
}

uint8_t IIC_Wait_ACK(void)
{
    uint8_t ack;
    IIC_SDA_1; IIC_Delay();
    IIC_SCL_1; IIC_Delay();
    ack = IIC_SDA_READ ? 1 : 0;
    IIC_SCL_0; IIC_Delay();
    return ack;
}
