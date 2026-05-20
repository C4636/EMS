/**
 * OLED 0.96寸 I2C SSD1306 驱动
 * 基于野火 F407 I2C EEPROM 例程模式改写
 */
#include "./oled/bsp_oled.h"
#include "./dwt_delay/bsp_dwt_delay.h"

static void OLED_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(OLED_I2C_GPIO_CLK, ENABLE);

    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;

    GPIO_InitStructure.GPIO_Pin   = OLED_SCL_PIN;
    GPIO_Init(OLED_SCL_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin   = OLED_SDA_PIN;
    GPIO_Init(OLED_SDA_PORT, &GPIO_InitStructure);

    GPIO_PinAFConfig(OLED_SCL_PORT, OLED_SCL_SOURCE, OLED_SCL_AF);
    GPIO_PinAFConfig(OLED_SDA_PORT, OLED_SDA_SOURCE, OLED_SDA_AF);
}

static void OLED_I2C_Config(void)
{
    I2C_InitTypeDef I2C_InitStructure;

    RCC_APB1PeriphClockCmd(OLED_I2C_CLK, ENABLE);

    I2C_InitStructure.I2C_Ack                 = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed          = OLED_I2C_SPEED;
    I2C_InitStructure.I2C_DutyCycle           = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_Mode                = I2C_Mode_I2C;
    I2C_InitStructure.I2C_OwnAddress1         = 0x78;
    I2C_Init(OLED_I2C, &I2C_InitStructure);
    I2C_Cmd(OLED_I2C, ENABLE);
}

void OLED_Init(void)
{
    OLED_GPIO_Config();
    OLED_I2C_Config();

    // TODO: SSD1306 初始化序列
    // OLED_WriteCmd(0xAE); // display off
    // OLED_WriteCmd(0x20); OLED_WriteCmd(0x00); // horizontal addressing
    // OLED_WriteCmd(0xA8); OLED_WriteCmd(0x3F); // mux ratio
    // ... etc
    // OLED_WriteCmd(0xAF); // display on
}

void OLED_Clear(void)             { /* TODO */ }
void OLED_ShowString(uint8_t x, uint8_t y, const char *str) { (void)x; (void)y; (void)str; }
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len) { (void)x; (void)y; (void)num; (void)len; }
void OLED_Refresh(void)           { /* TODO */ }
