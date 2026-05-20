/**
  ******************************************************************************
  * @file    main.c
  * @brief   EMS Integrated Project - STM32F407 BaTianHu
  *
  * KEYS:  KEY1 (PA0)  - cycle test mode (0->1->2->3->4->5->0)
  *        KEY2 (PC13) - trigger action in current mode
  *
  * Modes:
  *   0: Self Test (LED RGB blink + USART info)
  *   1: MQ135 + MQ2 gas sensors (ADC1 dual-channel DMA)
  *   2: OLED I2C display test (I2C1 bus verify)
  *   3: DHT11 temp/humidity (one-wire)
  *   4: TB6612FNG motor control (PWM speed + direction)
  *   5: All modules cycle (rotate every 2s)
  *
  * Pin Map:
  *   MQ135:  AO=PA4(ADC1_CH4)  DO=PC4
  *   MQ2:    AO=PA1(ADC1_CH1)  DO=PC5
  *   OLED:   SCL=PB6(I2C1)     SDA=PB7(I2C1)
  *   DHT11:  DATA=PA6
  *   TB6612: PWMA=PB0(TIM3_CH3) AIN1=PC0 AIN2=PC1 STBY=PC2
  *   USART1: TX=PA9  RX=PA10
  *   KEY:    K1=PA0  K2=PC13
  *   LED:    R=PF6  G=PF7  B=PF8
  ******************************************************************************
  */

#include "stm32f4xx.h"
#include "./usart/bsp_usart.h"
#include "./led/bsp_led.h"
#include "./key/bsp_key.h"
#include "./dwt_delay/bsp_dwt_delay.h"
#include "./dht11/bsp_dht11.h"
#include "./mq135/bsp_mq135.h"
#include "./mq2/bsp_mq2.h"
#include "./oled/bsp_oled.h"
#include "./tb6612/bsp_tb6612.h"

extern __IO uint16_t MQ_ADC_Values[2];

static volatile uint8_t test_mode      = 0;
static volatile uint8_t mode_changed   = 1;
static volatile uint8_t key2_triggered = 0;

/* ──────────────── Mode 0: Self Test ──────────────── */
static void Mode0_SelfTest(void)
{
    if (mode_changed) {
        mode_changed = 0;
        printf("\r\n========================================\r\n");
        printf("  Mode 0: System Self Test\r\n");
        printf("========================================\r\n");
        printf("  SysClk: 168 MHz\r\n");
        printf("  USART1: 115200-8-N-1 (printf ready)\r\n");
        printf("  LED:    PF6(R) PF7(G) PF8(B) - active low\r\n");
        printf("  KEY:    PA0(K1) PC13(K2) - active high\r\n");
        printf("  Delay:  DWT->CYCCNT us/ms ready\r\n");
        printf("\r\n  LED RGB blink test... Press KEY2 to reprint\r\n");
    }
    LED_RED;    DWT_Delay_ms(300);  LED_RGBOFF;
    LED_GREEN;  DWT_Delay_ms(300);  LED_RGBOFF;
    LED_BLUE;   DWT_Delay_ms(300);  LED_RGBOFF;
}

/* ──────────────── Mode 1: MQ135 + MQ2 ──────────────── */
static void Mode1_MQSensors(void)
{
    if (mode_changed) {
        mode_changed = 0;
        printf("\r\n========================================\r\n");
        printf("  Mode 1: MQ135 + MQ2 Gas Sensors\r\n");
        printf("========================================\r\n");
        printf("  MQ135: AO=PA4(ADC1_CH4) DO=PC4\r\n");
        printf("  MQ2:   AO=PA1(ADC1_CH1) DO=PC5\r\n");
        printf("  ADC1 dual-channel scan + DMA2_Stream0\r\n");
        printf("  ADC clk = PCLK2/4 = 21 MHz\r\n");
        printf("  Sample = 56 cycles\r\n");
        printf("\r\n  NOTE: sensors need 1-2 min warm-up\r\n");
        printf("  Press KEY2 to refresh reading\r\n");
    }

    float mq135_ppm = MQ135_Get_PPM();
    float mq2_ppm   = MQ2_Get_PPM();
    uint8_t mq135_do = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4);
    uint8_t mq2_do   = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_5);

    printf("\r\n  ---- Gas Sensor Readings ----\r\n");
    {
        int v1 = (int)((float)MQ_ADC_Values[1] / 4096.0f * 3.3f * 1000 + 0.5f);
        int p1 = (int)(mq135_ppm * 10 + 0.5f);
        int p1_dec = p1 >= 0 ? p1 % 10 : (-p1) % 10;
        printf("  MQ135: ADC=%4d (%d.%03dV) PPM=%d.%d DO=%d %s\r\n",
               MQ_ADC_Values[1],
               v1 / 1000, v1 % 1000,
               p1 / 10, p1_dec,
               mq135_do,
               mq135_do ? "OK" : "ALARM!");
    }
    {
        int v2 = (int)((float)MQ_ADC_Values[0] / 4096.0f * 3.3f * 1000 + 0.5f);
        int p2 = (int)(mq2_ppm * 10 + 0.5f);
        int p2_dec = p2 >= 0 ? p2 % 10 : (-p2) % 10;
        printf("  MQ2  : ADC=%4d (%d.%03dV) PPM=%d.%d DO=%d %s\r\n",
               MQ_ADC_Values[0],
               v2 / 1000, v2 % 1000,
               p2 / 10, p2_dec,
               mq2_do,
               mq2_do ? "OK" : "ALARM!");
    }

    if (mq135_ppm > 500 || mq2_ppm > 5000)
        { LED_RED; }
    else if (mq135_ppm > 100 || mq2_ppm > 1000)
        { LED_YELLOW; }
    else
        { LED_GREEN; }
}

/* ──────────────── Mode 2: OLED ──────────────── */
static void Mode2_OLED(void)
{
    if (mode_changed) {
        mode_changed = 0;
        printf("\r\n========================================\r\n");
        printf("  Mode 2: OLED 0.96\" I2C Display\r\n");
        printf("========================================\r\n");
        printf("  SCL=PB6  SDA=PB7 (I2C1, 400kHz)\r\n");
        printf("  SSD1306 addr: 0x3C\r\n");
        printf("  GPIO: open-drain, no pull (ext 4.7k pull-up needed)\r\n");
        printf("\r\n  I2C1 HW ready, press KEY2 to send test data\r\n");
        printf("  (SSD1306 init sequence TBD)\r\n");
    }

    if (key2_triggered) {
        key2_triggered = 0;

        I2C_GenerateSTART(I2C1, ENABLE);
        while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
        I2C_Send7bitAddress(I2C1, 0x3C, I2C_Direction_Transmitter);

        if (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
            printf("  [OK] OLED SSD1306 ACK (addr 0x3C)\r\n");
            LED_GREEN;

            I2C_SendData(I2C1, 0x00);
            while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
            I2C_SendData(I2C1, 0xAF);
            while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
            printf("  [OK] Sent Display ON cmd (0xAF)\r\n");
        } else {
            printf("  [FAIL] OLED no ACK! Check:\r\n");
            printf("    1. VCC->3.3V, GND->GND\r\n");
            printf("    2. SCL->PB6, SDA->PB7\r\n");
            printf("    3. Ext 4.7k pull-up to 3.3V\r\n");
            LED_RED;
        }
        I2C_GenerateSTOP(I2C1, ENABLE);

        OLED_ShowString(0, 0, "EMS Project");
        OLED_ShowString(0, 2, "F407 BaTianHu");
        OLED_Refresh();
    }
}

/* ──────────────── Mode 3: DHT11 ──────────────── */
static void Mode3_DHT11(void)
{
    if (mode_changed) {
        mode_changed = 0;
        printf("\r\n========================================\r\n");
        printf("  Mode 3: DHT11 Temperature & Humidity\r\n");
        printf("========================================\r\n");
        printf("  DATA=PA6 (ext 4.7k pull-up to 3.3V)\r\n");
        printf("\r\n  Press KEY2 to trigger one reading\r\n");
        printf("  (one-wire protocol TBD)\r\n");
    }

    if (key2_triggered) {
        key2_triggered = 0;
        uint8_t temp = 0, humi = 0;
        if (DHT11_Read_Data(&temp, &humi) == 0) {
            printf("  DHT11: Temp=%dC  Humidity=%d%%RH\r\n", temp, humi);
            if (temp > 30) { LED_RED;   }
            else           { LED_GREEN; }
        } else {
            printf("  [FAIL] DHT11 read failed! Check:\r\n");
            printf("    1. VCC->3.3V, GND->GND\r\n");
            printf("    2. DATA->PA6\r\n");
            printf("    3. Ext 4.7k pull-up on DATA to 3.3V\r\n");
            LED_RED;
        }
    }
}

/* ──────────────── Mode 4: TB6612FNG ──────────────── */
static void Mode4_TB6612(void)
{
    static uint16_t motor_speed = 30;

    if (mode_changed) {
        mode_changed = 0;
        motor_speed = 30;
        printf("\r\n========================================\r\n");
        printf("  Mode 4: TB6612FNG Motor Driver\r\n");
        printf("========================================\r\n");
        printf("  PWMA=PB0(TIM3_CH3) AIN1=PC0 AIN2=PC1 STBY=PC2\r\n");
        printf("  PWM freq: 10kHz (TIM3: 84MHz/(84x100))\r\n");
        printf("  Current speed: %d%%\r\n", motor_speed);
        printf("\r\n  KEY2: speed +15%% (wrap 0->15->...->90->0)\r\n");
        printf("  (PWM & direction functions TBD)\r\n");
    }

    if (key2_triggered) {
        key2_triggered = 0;
        motor_speed += 15;
        if (motor_speed > 100) motor_speed = 0;
        TB6612_SetSpeed(motor_speed);
        printf("  TB6612 speed: %d%%\r\n", motor_speed);
        if (motor_speed > 60)      { LED_RED;    }
        else if (motor_speed > 20) { LED_YELLOW; }
        else                       { LED_GREEN;  }
    }
}

/* ──────────────── Mode 5: All Modules ──────────────── */
static void Mode5_AllModules(void)
{
    if (mode_changed) {
        mode_changed = 0;
        printf("\r\n========================================\r\n");
        printf("  Mode 5: All Modules Rotation\r\n");
        printf("========================================\r\n");
        printf("  Press KEY2 to cycle display\r\n");
    }

    if (!key2_triggered) return;
    key2_triggered = 0;

    static uint8_t sub_phase = 0;

    switch (sub_phase) {
    case 0: {
        float mq135_ppm = MQ135_Get_PPM();
        float mq2_ppm   = MQ2_Get_PPM();
        int p1 = (int)(mq135_ppm * 10 + 0.5f);
        int p2 = (int)(mq2_ppm * 10 + 0.5f);
        printf("\r\n[Gas] MQ135=%d.%dppm  MQ2=%d.%dppm\r\n",
               p1 / 10, p1 >= 0 ? p1 % 10 : (-p1) % 10,
               p2 / 10, p2 >= 0 ? p2 % 10 : (-p2) % 10);
        printf("  MQ135 DO=%d  MQ2 DO=%d\r\n",
               GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4),
               GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_5));
        LED_RED;
        break;
    }
    case 1: {
        uint8_t dht11_temp = 0, dht11_humi = 0;
        if (DHT11_Read_Data(&dht11_temp, &dht11_humi) == 0)
            printf("[Env] Temp=%dC  Humidity=%d%%RH\r\n",
                   dht11_temp, dht11_humi);
        else
            printf("[Env] DHT11 not responding\r\n");
        LED_GREEN;
        break;
    }
    case 2:
        printf("[OLED] I2C1 bus ready, waiting for SSD1306 init\r\n");
        OLED_ShowString(0, 0, "All Sensors OK");
        OLED_Refresh();
        LED_BLUE;
        break;
    }
    if (++sub_phase >= 3) sub_phase = 0;
}

/* ─────────────────── main ─────────────────── */
int main(void)
{
    DWT_Delay_Init();
    USART_Config();
    LED_GPIO_Config();
    Key_Init();

    printf("\r\n\r\n");
    printf("+==========================================+\r\n");
    printf("|   EMS Project - STM32F407 BaTianHu       |\r\n");
    printf("|   Environment Monitoring System          |\r\n");
    printf("+==========================================+\r\n");
    printf("\r\nSysClk: %d MHz\r\n", SystemCoreClock / 1000000);
    printf("FW Lib: STM32F4xx_StdPeriph_Driver V1.8.0\r\n");
    printf("\r\nInitializing modules...\r\n");

    DHT11_Init();
    printf("  [OK] DHT11 GPIO ready (PA6)\r\n");

    MQ135_Init();
    printf("  [OK] MQ135+MQ2 ADC1 DMA dual-chan ready\r\n");

    MQ2_Init();
    printf("  [OK] MQ2 GPIO ready\r\n");

    OLED_Init();
    printf("  [OK] OLED I2C1 ready (400kHz)\r\n");

    TB6612_Init();
    printf("  [OK] TB6612 GPIO ready\r\n");

    printf("\r\nAll modules initialized!\r\n");
    printf("\r\n========================================\r\n");
    printf("  KEYS:\r\n");
    printf("  KEY1 (PA0)  - cycle mode (0~5)\r\n");
    printf("  KEY2 (PC13) - trigger action\r\n");
    printf("========================================\r\n\r\n");

    Mode0_SelfTest();

    while (1) {
        if (Key_Scan(GPIOA, GPIO_Pin_0) == KEY_ON) {
            test_mode++;
            if (test_mode > 5) test_mode = 0;
            mode_changed = 1;
            LED_RGBOFF;
        }
        if (Key_Scan(GPIOC, GPIO_Pin_13) == KEY_ON) {
            key2_triggered = 1;
        }
        switch (test_mode) {
        case 0: Mode0_SelfTest();    break;
        case 1: Mode1_MQSensors();   break;
        case 2: Mode2_OLED();        break;
        case 3: Mode3_DHT11();       break;
        case 4: Mode4_TB6612();      break;
        case 5: Mode5_AllModules();  break;
        default: test_mode = 0;      break;
        }
    }
}
