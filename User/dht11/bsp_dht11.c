/**
 * DHT11 driver — ported from Wildfire F407 official example
 * Pin: PA6 (adapted from original PE3)
 */
#include "./dht11/bsp_dht11.h"
#include "./dwt_delay/core_delay.h"

#define DHT11_DELAY_US(us)  CPU_TS_Tmr_Delay_US(us)

void DHT11_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(DHT11_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = DHT11_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_PORT, &GPIO_InitStructure);
}

static void DHT11_Mode_IPU(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin   = DHT11_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_PORT, &GPIO_InitStructure);
}

static void DHT11_Mode_Out_PP(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin   = DHT11_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_PORT, &GPIO_InitStructure);
}

static uint8_t Read_Byte(void)
{
    uint8_t i, temp = 0;
    for (i = 0; i < 8; i++) {
        while (DHT11_DATA_IN() == Bit_RESET);
        DHT11_DELAY_US(40);
        if (DHT11_DATA_IN() == Bit_SET) {
            while (DHT11_DATA_IN() == Bit_SET);
            temp |= (uint8_t)(0x01 << (7 - i));
        } else {
            temp &= (uint8_t)~(0x01 << (7 - i));
        }
    }
    return temp;
}

uint8_t Read_DHT11(DHT11_Data_TypeDef *DHT11_Data)
{
    uint16_t count;

    /* start signal: low 20ms */
    DHT11_Mode_Out_PP();
    DHT11_DATA_OUT(DHT11_LOW);
    DHT11_DELAY_US(20000);

    /* pull high 30us */
    DHT11_DATA_OUT(DHT11_HIGH);
    DHT11_DELAY_US(30);

    /* switch to input, read response */
    DHT11_Mode_IPU();

    if (DHT11_DATA_IN() == Bit_RESET) {
        count = 0;
        while (DHT11_DATA_IN() == Bit_RESET) {
            count++;
            if (count > 1000) return 0;
            DHT11_DELAY_US(10);
        }
        count = 0;
        while (DHT11_DATA_IN() == Bit_SET) {
            count++;
            if (count > 1000) return 0;
            DHT11_DELAY_US(10);
        }

        DHT11_Data->humi_int  = Read_Byte();
        DHT11_Data->humi_deci = Read_Byte();
        DHT11_Data->temp_int  = Read_Byte();
        DHT11_Data->temp_deci = Read_Byte();
        DHT11_Data->check_sum = Read_Byte();

        DHT11_Mode_Out_PP();
        DHT11_DATA_OUT(DHT11_HIGH);

        if (DHT11_Data->check_sum == DHT11_Data->humi_int +
                                      DHT11_Data->humi_deci +
                                      DHT11_Data->temp_int +
                                      DHT11_Data->temp_deci)
            return 1;
        else
            return 0;
    }
    return 0;
}
