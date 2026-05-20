/**
 * DHT11 驱动 — 待实现
 * 引脚: PA6 (需外接 4.7k 上拉电阻)
 * 依赖: bsp_dwt_delay (微秒级延时)
 */
#include "./dht11/bsp_dht11.h"
#include "./dwt_delay/bsp_dwt_delay.h"

void DHT11_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(DHT11_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Pin = DHT11_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_PORT, &GPIO_InitStructure);

    GPIO_SetBits(DHT11_PORT, DHT11_PIN);
}

uint8_t DHT11_Read_Data(uint8_t *temp, uint8_t *humi)
{
    // TODO: implement DHT11 one-wire protocol
    if (temp) *temp = 0;
    if (humi) *humi = 0;
    return 1;   // return failure until protocol is implemented
}
