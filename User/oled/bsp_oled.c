/**
 * OLED SSD1306 128x64 I2C driver
 * Ported from Wildfire F407 official example
 * Uses hardware I2C1 (PB6=SCL, PB7=SDA)
 */
#include "./oled/bsp_oled.h"
#include "./iic/bsp_iic_debug.h"
#include "./oled/bsp_oled_codetab.h"

extern const uint8_t F6x8[][6];
extern const uint8_t F8X16[];
extern const uint8_t F16x16[];
extern const uint8_t BMP1[];

/* ──────── hardware I2C: ctrl + data in one transaction ──────── */
static void IIC_Write_Byte(uint8_t ctrl, uint8_t data)
{
    while (I2C_GetFlagStatus(IIC_NUM, I2C_FLAG_BUSY));
    I2C_GenerateSTART(IIC_NUM, ENABLE);
    while (!I2C_CheckEvent(IIC_NUM, I2C_EVENT_MASTER_MODE_SELECT));
    I2C_Send7bitAddress(IIC_NUM, OLED_ID, I2C_Direction_Transmitter);
    while (!I2C_CheckEvent(IIC_NUM, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
    I2C_SendData(IIC_NUM, ctrl);
    while (!I2C_CheckEvent(IIC_NUM, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    I2C_SendData(IIC_NUM, data);
    while (!I2C_CheckEvent(IIC_NUM, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    I2C_GenerateSTOP(IIC_NUM, ENABLE);
}

static void Oled_Write_Cmd(uint8_t cmd)
{
    IIC_Write_Byte(OLED_WR_CMD, cmd);
}

static void Oled_Write_Data(uint8_t data)
{
    IIC_Write_Byte(OLED_WR_DATA, data);
}

/* ──────── SSD1306 init sequence ──────── */
void OLED_Init(void)
{
    Oled_Write_Cmd(0xAE); /* display off */
    Oled_Write_Cmd(0x81); Oled_Write_Cmd(0xFF); /* contrast */
    Oled_Write_Cmd(0xA4); /* normal display */
    Oled_Write_Cmd(0xA6); /* normal (non-inverted) */
    Oled_Write_Cmd(0x2E); /* deactivate scroll */
    Oled_Write_Cmd(0x20); Oled_Write_Cmd(0x10); /* page addressing */
    Oled_Write_Cmd(0xB0); /* start page */
    Oled_Write_Cmd(0x00); /* column low */
    Oled_Write_Cmd(0x10); /* column high */
    Oled_Write_Cmd(0x40); /* display start line */
    Oled_Write_Cmd(0xA1); /* segment remap (left-right mirror) */
    Oled_Write_Cmd(0xA8); Oled_Write_Cmd(0x3F); /* mux 64 */
    Oled_Write_Cmd(0xC8); /* COM scan direction remap */
    Oled_Write_Cmd(0xD3); Oled_Write_Cmd(0x00); /* COM offset */
    Oled_Write_Cmd(0xDA); Oled_Write_Cmd(0x12); /* COM pins hardware config */
    Oled_Write_Cmd(0xD5); Oled_Write_Cmd(0x80); /* osc freq */
    Oled_Write_Cmd(0xD9); Oled_Write_Cmd(0x22); /* pre-charge period */
    Oled_Write_Cmd(0xDB); Oled_Write_Cmd(0x20); /* VCOMH deselect */
    Oled_Write_Cmd(0x8D); Oled_Write_Cmd(0x14); /* charge pump enable */
    Oled_Write_Cmd(0xAF); /* display on */
}

void OLED_SetPos(uint8_t x, uint8_t y)
{
    Oled_Write_Cmd(0xB0 + y);
    Oled_Write_Cmd(((x & 0xF0) >> 4) | 0x10);
    Oled_Write_Cmd(x & 0x0F);
}

void OLED_Fill(uint8_t fill_data)
{
    uint8_t m, n;
    for (m = 0; m < 8; m++) {
        Oled_Write_Cmd(0xB0 + m);
        Oled_Write_Cmd(0x00);
        Oled_Write_Cmd(0x10);
        for (n = 0; n < 128; n++) Oled_Write_Data(fill_data);
    }
}

void OLED_CLS(void) { OLED_Fill(0x00); }

void OLED_ClearPage(uint8_t y)
{
    uint8_t i;
    OLED_SetPos(0, y);
    for (i = 0; i < 128; i++) Oled_Write_Data(0x00);
}

void OLED_FillBytes(uint8_t x, uint8_t y, uint8_t len, uint8_t data)
{
    uint8_t i;
    OLED_SetPos(x, y);
    for (i = 0; i < len; i++) Oled_Write_Data(data);
}

void OLED_ON(void)
{
    Oled_Write_Cmd(0x8D); Oled_Write_Cmd(0x14);
    Oled_Write_Cmd(0xAF);
}

void OLED_OFF(void)
{
    Oled_Write_Cmd(0x8D); Oled_Write_Cmd(0x10);
    Oled_Write_Cmd(0xAE);
}

void OLED_Refresh(void) { /* page-mode SSD1306 auto-refreshes */ }

/* ──────── ASCII string display ──────── */
void OLED_ShowStr(uint8_t x, uint8_t y, const char *ch, uint8_t text_size)
{
    uint8_t c, i, j;
    uint8_t l_str_len = 0;

    while (ch[l_str_len] != '\0') l_str_len++;

    if (text_size == 1) {
        /* 6x8 font */
        for (i = 0; i < l_str_len; i++) {
            c = ch[i] - ' ';
            if (x > 126) { x = 0; y++; }
            OLED_SetPos(x, y);
            for (j = 0; j < 6; j++) Oled_Write_Data(F6x8[c][j]);
            x += 6;
        }
    } else {
        /* 8x16 font (two pages per char) */
        for (i = 0; i < l_str_len; i++) {
            c = ch[i] - ' ';
            if (x > 120) { x = 0; y++; }
            OLED_SetPos(x, y);
            for (j = 0; j < 8; j++) Oled_Write_Data(F8X16[c * 16 + j]);
            OLED_SetPos(x, y + 1);
            for (j = 0; j < 8; j++) Oled_Write_Data(F8X16[c * 16 + j + 8]);
            x += 8;
        }
    }
}

/* ──────── Chinese 16x16 ──────── */
void OLED_ShowCN(uint8_t x, uint8_t y, uint8_t n)
{
    uint8_t j;
    OLED_SetPos(x, y);
    for (j = 0; j < 16; j++) Oled_Write_Data(F16x16[n * 32 + j]);
    OLED_SetPos(x, y + 1);
    for (j = 0; j < 16; j++) Oled_Write_Data(F16x16[n * 32 + j + 16]);
}

/* ──────── BMP ──────── */
void OLED_DrawBMP(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, const uint8_t BMP[])
{
    uint32_t j = 0;
    uint8_t x, y;
    for (y = y0; y <= y1; y++) {
        OLED_SetPos(x0, y);
        for (x = x0; x <= x1; x++) Oled_Write_Data(BMP[j++]);
    }
}
