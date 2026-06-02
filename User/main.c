/**
  ******************************************************************************
  * @file    main.c
  * @brief   EMS Environmental Monitor - STM32F407 BaTianHu
  *
  *  Real-time monitoring: DHT11 temp/humidity, MQ135 air quality, MQ2 smoke.
  *  OLED displays all readings. Fan (TB6612FNG) auto-starts on alarm.
  *
  *  Thresholds:
  *    Temp  >  30 C    -> ALARM
  *    MQ135 >  500 ppm -> ALARM
  *    MQ2   > 5000 ppm -> ALARM
  *    Any alarm -> fan ON at 60%
  *
  *  Pin Map:
  *    DHT11:  DATA=PA6
  *    MQ135:  AO=PA4(ADC1_CH4)  DO=PC4
  *    MQ2:    AO=PA1(ADC1_CH1)  DO=PC5
  *    OLED:   SCL=PB6(I2C1)     SDA=PB7(I2C1)
  *    TB6612: PWMA=PB0(TIM3_CH3) AIN1=PC0 AIN2=PC1 STBY=PC2
  *    USART1: TX=PA9  RX=PA10
  *    LED:    R=PF6  G=PF7  B=PF8
  ******************************************************************************
  */

#include "stm32f4xx.h"
#include "./usart/bsp_usart.h"
#include "./led/bsp_led.h"
#include "./dwt_delay/bsp_dwt_delay.h"
#include "./dht11/bsp_dht11.h"
#include "./mq135/bsp_mq135.h"
#include "./mq2/bsp_mq2.h"
#include "./iic/bsp_iic_debug.h"
#include "./oled/bsp_oled.h"
#include "./tb6612/bsp_tb6612.h"

extern __IO uint16_t MQ_ADC_Values[2];

/* alarm thresholds */
#define TEMP_ALARM      30
#define MQ135_ALARM     500
#define MQ2_ALARM       5000
#define FAN_SPEED       60

/* system state */
static DHT11_Data_TypeDef dht11;
static uint8_t  alarm_active      = 0;
static uint8_t  prev_alarm_active = 0;
static uint8_t  fan_running       = 0;
static uint8_t  prev_dht11_ok     = 1;

/* helpers */
static void OLED_ShowInt(uint8_t x, uint8_t y, int val, uint8_t size)
{
    char s[12]; s[0]=0;
    int j=0, n=val, neg=0;
    if (n<0) { neg=1; n=-n; }
    do { s[j++]='0'+n%10; n/=10; } while(n);
    if (neg) s[j++]='-';
    for(int k=0;k<j/2;k++) { char t=s[k]; s[k]=s[j-1-k]; s[j-1-k]=t; }
    s[j]=0;
    OLED_ShowStr(x, y, s, size);
}

static void OLED_ShowFloat1(uint8_t x, uint8_t y, float val, uint8_t size)
{
    char s[16];
    int ipart = (int)val;
    int dpart = (int)((val - ipart) * 10.0f + 0.5f);
    if (dpart < 0) dpart = -dpart;
    int j=0, n=ipart, neg=0;
    if (n<0) { neg=1; n=-n; }
    if (n==0) { s[j++]='0'; }
    while(n) { s[j++]='0'+n%10; n/=10; }
    if (neg) s[j++]='-';
    for(int k=0;k<j/2;k++) { char t=s[k]; s[k]=s[j-1-k]; s[j-1-k]=t; }
    s[j++]='.'; s[j++]='0'+dpart; s[j]=0;
    OLED_ShowStr(x, y, s, size);
}

static void OLED_ShowTitle(void)
{
    OLED_ShowStr(0, 0, "EMS MONITOR", 2);  /* 8x16, two pages */
}

static void OLED_ShowEnv(float t_int, float t_dec, float h_int, float h_dec)
{
    /* ── temperature (page 2) ── */
    OLED_ShowStr(0, 2, "T:", 1);
    OLED_FillBytes(12, 2, 4, 0x00);   /* clear "T:"→int gap */
    OLED_FillBytes(16, 2, 18, 0x00);  /* clear int field (max 3 digits) */
    OLED_ShowInt(16, 2, (int)t_int, 1);
    OLED_FillBytes(34, 2, 6, 0x00);   /* clear int→"." gap */
    OLED_ShowStr(40, 2, ".", 1);
    OLED_FillBytes(46, 2, 2, 0x00);   /* clear "."→decimal gap */
    OLED_ShowInt(48, 2, (int)t_dec, 1);
    OLED_FillBytes(54, 2, 10, 0x00);  /* clear decimal→"C" gap */
    OLED_ShowStr(64, 2, "C", 1);
    OLED_FillBytes(70, 2, 10, 0x00);  /* clear "C"→fan gap */

    /* ── humidity (page 4) ── */
    OLED_ShowStr(0, 4, "H:", 1);
    OLED_FillBytes(12, 4, 4, 0x00);
    OLED_FillBytes(16, 4, 18, 0x00);
    OLED_ShowInt(16, 4, (int)h_int, 1);
    OLED_FillBytes(34, 4, 6, 0x00);
    OLED_ShowStr(40, 4, ".", 1);
    OLED_FillBytes(46, 4, 2, 0x00);
    OLED_ShowInt(48, 4, (int)h_dec, 1);
    OLED_FillBytes(54, 4, 10, 0x00);
    OLED_ShowStr(64, 4, "%", 1);
    OLED_FillBytes(70, 4, 58, 0x00);  /* clear rest of page 4 */
}

static void OLED_ShowGas(float mq135, float mq2)
{
    OLED_ShowStr(0, 6, "A:", 1);
    OLED_ShowFloat1(16, 6, mq135, 1);
    OLED_ShowStr(64, 6, "ppm", 1);

    /* MQ2 shown via UART only (OLED line 6 already used) */
    (void)mq2;
}

static void OLED_ShowAlarm(void)
{
    OLED_ShowStr(0, 6, "** ALARM **", 2);
}

static void OLED_ShowFan(uint8_t on)
{
    if (on)
        OLED_ShowStr(80, 2, "FAN:ON ", 1);  /* trailing space = 7 chars, matches "FAN:OFF" */
    else
        OLED_ShowStr(80, 2, "FAN:OFF", 1);
}

/* check alarm conditions, return 1 if any alarm */
static uint8_t CheckAlarm(float temp, float mq135, float mq2,
                          uint8_t mq135_do, uint8_t mq2_do)
{
    uint8_t alarm = 0;
    if (temp > TEMP_ALARM) {
        printf("  [ALARM] Temp %.1fC > %dC\r\n", temp, TEMP_ALARM);
        alarm = 1;
    }
    if (mq135 > MQ135_ALARM || mq135_do == 0) {
        printf("  [ALARM] MQ135 %.1fppm > %dppm\r\n", mq135, MQ135_ALARM);
        alarm = 1;
    }
    if (mq2 > MQ2_ALARM || mq2_do == 0) {
        printf("  [ALARM] MQ2 %.1fppm > %dppm\r\n", mq2, MQ2_ALARM);
        alarm = 1;
    }
    return alarm;
}

/* ─────────────────── main ─────────────────── */
int main(void)
{
    float temp, humi, mq135_ppm, mq2_ppm;
    uint8_t mq135_do, mq2_do;

    /* init hardware */
    DWT_Delay_Init();
    USART_Config();
    LED_GPIO_Config();

    printf("\r\n+==========================================+\r\n");
    printf("|  EMS Environment Monitor                 |\r\n");
    printf("|  STM32F407 BaTianHu  FAN:TB6612          |\r\n");
    printf("+==========================================+\r\n\r\n");

    DHT11_GPIO_Config();
    printf("[OK] DHT11 ready\r\n");

    MQ135_Init();   /* ADC1 dual-chan DMA */
    MQ2_Init();
    printf("[OK] MQ135+MQ2 ADC ready\r\n");

    IIC_GPIO_Config();
    OLED_Init();
    OLED_CLS();
    OLED_ShowTitle();
    printf("[OK] OLED ready\r\n");

    TB6612_Init();
    printf("[OK] TB6612 PWM ready\r\n\r\n");

    printf("Monitoring started...\r\n\r\n");

    while (1)
    {
        /* read DHT11 */
        uint8_t dht11_ok = Read_DHT11(&dht11);
        if (dht11_ok) {
            temp = (float)dht11.temp_int + (float)dht11.temp_deci * 0.1f;
            humi = (float)dht11.humi_int + (float)dht11.humi_deci * 0.1f;
        } else {
            temp = 0; humi = 0;
        }

        /* read MQ sensors */
        mq135_ppm = MQ135_Get_PPM();
        mq2_ppm   = MQ2_Get_PPM();
        mq135_do  = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4);
        mq2_do    = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_5);

        /* check alarms */
        alarm_active = CheckAlarm(temp, mq135_ppm, mq2_ppm, mq135_do, mq2_do);

        /* fan control */
        if (alarm_active) {
            if (!fan_running) {
                TB6612_Forward();
                TB6612_SetSpeed(FAN_SPEED);
                fan_running = 1;
                printf(">>> Fan STARTED (speed %d%%)\r\n", FAN_SPEED);
            }
            { LED_RED; }
        } else {
            if (fan_running) {
                TB6612_SetSpeed(0);
                TB6612_Stop();
                fan_running = 0;
                printf(">>> Fan STOPPED\r\n");
            }
            { LED_GREEN; }
        }

        /* UART output */
        printf("T=%.1fC H=%.1f%% MQ135=%.1fppm MQ2=%.1fppm DO135=%d DO2=%d %s\r\n",
               temp, humi, mq135_ppm, mq2_ppm, mq135_do, mq2_do,
               alarm_active ? "[ALARM!]" : "");

        /* OLED update */
        OLED_SetPos(0, 0);
        OLED_ShowTitle();

        if (dht11_ok) {
            OLED_ShowEnv(dht11.temp_int, dht11.temp_deci,
                         dht11.humi_int, dht11.humi_deci);
        } else {
            OLED_ShowStr(0, 2, "DHT11: --", 1);
        }

        if (alarm_active) {
            OLED_ShowAlarm();
        } else {
            /* alarm just cleared → clear page 7 (bottom halves of 8x16 text) */
            if (prev_alarm_active) OLED_ClearPage(7);
            OLED_ShowGas(mq135_ppm, mq2_ppm);
        }
        OLED_ShowFan(fan_running);
        prev_alarm_active = alarm_active;

        DWT_Delay_ms(2000);
    }
}
