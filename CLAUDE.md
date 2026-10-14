# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

EMS (Environmental Monitoring System) on a YeaFire STM32F407ZGTx "BaTianHu" board. Real-time DHT11 temp/humidity, MQ135 air quality, MQ2 smoke detection, with I2C OLED display and TB6612FNG-driven DC fan for automatic ventilation.

## Build System

This is a **Keil MDK-ARM 5.38** project — there is no Makefile, CMake, or CLI build. There are no tests, no CI, and no build scripts of any kind. Open `Project\RVMDK（uv5）\EMS_Project.uvprojx` in Keil.

Key compiler settings:
- **Compiler**: ARMCC V5.06 update 7 (NOT GCC, NOT ARMCC V6/AC6)
- **Defines**: `USE_STDPERIPH_DRIVER,STM32F40_41xxx`
- **MicroLIB**: enabled (required for reduced code size and `printf` redirection)
- **Optimization**: -O2
- **Include paths**: `User; Libraries\...\inc; Libraries\CMSIS\Include; Libraries\...\STM32F4xx\Include`

To build: F7 (compile) → F8 (download via Fire CMSIS-DAP SWD).

Keil project file groups: STARTUP (startup_stm32f40xx.s), CMSIS (system_stm32f4xx.c), STM32F4xx_StdPeriph_Driver (38 .c files; dma2d/fmc/ltdc excluded from build), USER (main.c + 12 bsp sources + stm32f4xx_it.c), DOC (readme).

## Pin Map

```
DHT11:  DATA=PA6
MQ135:  AO=PA4(ADC1_CH4)  DO=PC4
MQ2:    AO=PA1(ADC1_CH1)  DO=PC5
OLED:   SCL=PB6(I2C1)     SDA=PB7(I2C1)   addr=0x78
TB6612: PWMA=PB0(TIM3_CH3) AIN1=PC0 AIN2=PC1 STBY=PC2
USART1: TX=PA9  RX=PA10   (115200-8-N-1)
LED:    R=PF6  G=PF7  B=PF8   (active-low: LED_RED macro drives PF6 low)
Key:    KEY1=PA0  KEY2=PC13   (not used in main loop)
```

External pull-ups required: DHT11 DATA (4.7kΩ to 3.3V), OLED SCL/SDA (4.7kΩ to 3.3V). MQ sensors need 5V heater power and 1-2 minute warm-up; their AO pins output 0-5V — voltage divider recommended for 3.3V ADC input.

## Code Architecture

### ADC1 DMA — shared between MQ135 and MQ2

`MQ135_Init()` in [bsp_mq135.c](User/mq135/bsp_mq135.c) configures ADC1 with dual-channel DMA scan mode for **both** sensors (it configures PA4 and PA1 as analog inputs, and PC4/PC5 as digital inputs for DO pins). `MQ2_Init()` only configures GPIO input for the DO pin — it does NOT touch ADC/DMA. The shared DMA buffer `MQ_ADC_Values[2]` holds:
- `[0]` = MQ2 (ADC1_CH1, PA1)
- `[1]` = MQ135 (ADC1_CH4, PA4)

`main.c` accesses this buffer via `extern __IO uint16_t MQ_ADC_Values[2]`.

**Because of this**, `MQ135_Init()` must be called before `MQ2_Init()` in main (the current code does this correctly).

### I2C dual-mode (hardware + software)

[bsp_iic_debug.h](User/iic/bsp_iic_debug.h) defines `IIC_SELECT`:
- `1` (default): hardware I2C1 at 400kHz on PB6/PB7
- `0`: software (bit-banged) I2C on the same pins — used as a fallback when hardware I2C has issues

All I2C public functions (`IIC_Start`, `IIC_SendByte`, etc.) transparently switch implementation based on this macro.

### DWT CYCCNT — global timing backbone

`core_delay.c/h` directly accesses Cortex-M4 DWT Cycle Counter registers (`DWT_CYCCNT`, `DEMCR`). This is the only microsecond-precision delay source used by DHT11's one-wire protocol and by the main loop. `bsp_dwt_delay.c/h` wraps it as `DWT_Delay_ms()` for convenience. System clock is 168MHz, so 1 tick = ~6ns.

### printf redirection

[bsp_usart.c](User/usart/bsp_usart.c) redirects `printf` to USART1 by overriding `fputc()`. MicroLIB is required for this to work — without it, the standard `fputc` takes precedence and serial output is broken.

### Module initialization order in main()

Must follow this order because of implicit dependencies:
1. `DWT_Delay_Init()` — needed by DHT11, OLED
2. `USART_Config()` — printf works after this
3. `LED_GPIO_Config()`
4. `DHT11_GPIO_Config()`
5. `MQ135_Init()` — configures ADC1+DMA (shared with MQ2)
6. `MQ2_Init()` — GPIO only
7. `IIC_GPIO_Config()` → `OLED_Init()`
8. `TB6612_Init()` — TIM3_CH3 PWM at 10kHz

### Alarm system

Alarm thresholds are defined as macros at the top of [main.c](User/main.c):

| Threshold | Value | Meaning |
|-----------|-------|---------|
| `TEMP_ALARM` | 30 | Temperature > 30°C |
| `MQ135_ALARM` | 500 | MQ135 PPM > 500 |
| `MQ2_ALARM` | 5000 | MQ2 PPM > 5000 |
| `FAN_SPEED` | 60 | Fan duty cycle 60% (0-100) |

`CheckAlarm()` evaluates five conditions (temp, MQ135 PPM, MQ2 PPM, MQ135 DO pin, MQ2 DO pin). DO pins are active-low (LOW = gas concentration exceeded the hardware potentiometer threshold). Any single condition triggers alarm.

Alarm state transitions:
- **Normal → Alarm**: sets LED red, starts fan forward at `FAN_SPEED`%, OLED shows `"** ALARM **"` on page 6-7 (8×16 font)
- **Alarm → Normal**: stops fan (short brake), sets LED green, clears OLED page 7 via `OLED_ClearPage(7)` to remove bottom halves of 8×16 alarm text, resumes gas display
- **Fan already running + still alarming**: no action (avoids redundant start/stop)
- **DHT11 read failure**: `temp=0, humi=0`, OLED shows `"DHT11: --"` — alarm may trigger on temp=0 but that's below 30°C so it won't

### TB6612FNG fan control

- **Forward**: AIN1=0, AIN2=1 (direction)
- **Backward**: AIN1=1, AIN2=0
- **Stop**: AIN1=0, AIN2=0 (**short brake** — both low shorts the motor, providing active braking unlike coast)
- **Speed**: directly writes `TIM3->CCR3` (0-100), PWM at 10kHz (PSC=84, ARR=100)
- **STBY**: tied high in `TB6612_Init()` (always enabled)

### Sensor constants and calibration

Gas PPM calculation uses a power-law model: `PPM = A × (Rs/R0)^B`, with sensor-specific constants in each header file. The `Rs` (sensor resistance) is derived from `Vout = ADC_Value/4096 × Vc`. MQ sensors require 1-2 minutes of preheating (5V heater) before readings stabilize.

Sensor constants:
- **MQ135**: RL=1kΩ, R0=2kΩ, A=4.17, B=-2.28
- **MQ2**: RL=10kΩ, R0=20kΩ, A=43.03, B=-1.66

### OLED SSD1306 — page buffer residue prevention

SSD1306 operates in page-addressing mode: writes only touch the exact columns specified, leaving unwritten columns with their previous pixel values. This causes "ghosting" when content shrinks (shorter text, fewer digits). The following patterns exist to prevent residue:

- **`OLED_ClearPage(y)`**: clears an entire 128-column page (8 pixel rows). Used on alarm→normal transition to clear page 7 (bottom halves of 8×16 alarm text).
- **`OLED_FillBytes(x, y, len, 0x00)`**: clears `len` columns starting at `(x, y)`. Used in `OLED_ShowEnv()` to clear every gap between fixed field positions and the variable-width integer fields — this handles both digit-count changes and DHT11 error-text recovery.
- **Fan status padding**: `"FAN:ON "` includes a trailing space so both ON/OFF strings occupy exactly 7 characters (42 pixels), preventing residue on state change.

When adding new OLED content, always consider: does the new content fully cover the worst-case extent of the previous content? If not, use `OLED_FillBytes` to clear the difference.

### Unused modules

The `bsp_key` module (PA0/PC13) exists in [User/key/](User/key/) but is **not called anywhere in main()**. It was part of an earlier demo-phase implementation. The key IRQ handlers are also not wired up in `stm32f4xx_it.c`. If you need button input, you'll need to add the init call and wire the EXTI interrupts.

## Main Loop Data Flow

```
while(1) every 2 seconds:
  1. Read_DHT11(&dht11)           → populate dht11 struct, or return 0 on checksum/timeout failure
  2. MQ135_Get_PPM()              → reads MQ_ADC_Values[1] from DMA, computes PPM via power law
  3. MQ2_Get_PPM()                → reads MQ_ADC_Values[0] from DMA, computes PPM via power law
  4. GPIO_ReadInputDataBit()      → read MQ135 DO (PC4) and MQ2 DO (PC5)
  5. CheckAlarm()                 → evaluate 5 conditions, printf alarm messages, return 0/1
  6. Fan control                  → start/stop based on alarm_active state (with edge detection)
  7. LED control                  → { LED_RED; } or { LED_GREEN; } (braces required — these are multi-statement macros)
  8. printf() to USART1           → single-line CSV-style: "T=... H=... MQ135=... MQ2=... DO135=... DO2=... [ALARM!]"
  9. OLED update                  → title, env data (or DHT11 error), gas data (or alarm banner), fan status
```

The 2-second delay is `DWT_Delay_ms(2000)` — a busy-wait that blocks the CPU.

## Peripheral Clock Speeds

```
HSE 25MHz → PLL (M=25,N=336,P=2) → SYSCLK=168MHz
AHB1 = 168MHz (GPIO, DMA2)
APB1 = 42MHz  → TIM3_CLK = 2×APB1 = 84MHz
APB2 = 84MHz  → ADC_CLK = APB2/4 = 21MHz
```

FPU (CP10/CP11) is enabled automatically by `SystemInit()` in the standard library startup sequence.

## Fault Handlers

All Cortex-M4 fault handlers in [stm32f4xx_it.c](User/stm32f4xx_it.c) (HardFault, MemManage, BusFault, UsageFault) are infinite `while(1)` loops with no logging or recovery. If the board locks up silently, a fault handler was likely hit — use the debugger to inspect the fault registers (CFSR, HFSR, MMAR, BFAR) at the trapped PC.

## Version Pinning

- **ST firmware**: STM32F4xx_StdPeriph_Driver V1.8.0 (vendored in `Libraries/`)
- **ARM Compiler**: V5.06 update 7 — do NOT use AC6/ARMClang, the code uses ARMCC-specific syntax (e.g. `__IO`, `__attribute__` patterns from CMSIS)
- **IDE**: Keil MDK 5.38
- **Device pack**: Keil.STM32F4xx_DFP.2.15.0
