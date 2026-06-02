# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

EMS (Environmental Monitoring System) on a YeaFire STM32F407ZGTx "BaTianHu" board. Real-time DHT11 temp/humidity, MQ135 air quality, MQ2 smoke detection, with I2C OLED display and TB6612FNG-driven DC fan for automatic ventilation.

## Build System

This is a **Keil MDK-ARM 5.38** project — there is no Makefile, CMake, or CLI build. Open `Project\RVMDK（uv5）\EMS_Project.uvprojx` in Keil.

Key compiler settings:
- **Compiler**: ARMCC V5.06 update 7 (NOT GCC, NOT ARMCC V6/AC6)
- **Defines**: `USE_STDPERIPH_DRIVER,STM32F40_41xxx`
- **MicroLIB**: enabled (required for reduced code size)
- **Optimization**: -O2
- **Include paths**: `User; Libraries\...\inc; Libraries\CMSIS\Include; Libraries\...\STM32F4xx\Include`

To build: F7 (compile) → F8 (download via Fire CMSIS-DAP SWD).

## Code Architecture

### ADC1 DMA — shared between MQ135 and MQ2

`MQ135_Init()` in [bsp_mq135.c](User/mq135/bsp_mq135.c) configures ADC1 with dual-channel DMA scan mode for **both** sensors. `MQ2_Init()` only configures GPIO input for the DO pin — it does NOT touch ADC/DMA. The shared DMA buffer `MQ_ADC_Values[2]` holds:
- `[0]` = MQ2 (ADC1_CH1, PA1)
- `[1]` = MQ135 (ADC1_CH4, PA4)

**Because of this**, `MQ135_Init()` must be called before `MQ2_Init()` in main (the current code does this correctly).

### I2C dual-mode (hardware + software)

[bsp_iic_debug.h](User/iic/bsp_iic_debug.h) defines `IIC_SELECT`:
- `1` (default): hardware I2C1 at 400kHz on PB6/PB7
- `0`: software (bit-banged) I2C on the same pins — used as a fallback when hardware I2C has issues

All I2C public functions (`IIC_Start`, `IIC_SendByte`, etc.) transparently switch implementation based on this macro.

### DWT CYCCNT — global timing backbone

`core_delay.c/h` directly accesses Cortex-M4 DWT Cycle Counter registers (`DWT_CYCCNT`, `DEMCR`). This is the only microsecond-precision delay source used by DHT11's one-wire protocol and by the main loop. `bsp_dwt_delay.c/h` wraps it as `DWT_Delay_ms()` for convenience. System clock is 168MHz, so 1 tick = ~6ns.

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

### Sensor constants and calibration

Gas PPM calculation uses a power-law model: `PPM = A × (Rs/R0)^B`, with sensor-specific constants in each header file. The `Rs` (sensor resistance) is derived from `Vout = ADC_Value/4096 × Vc`. MQ sensors require 1-2 minutes of preheating (5V heater) before readings stabilize.

### OLED SSD1306 — page buffer residue prevention

SSD1306 operates in page-addressing mode: writes only touch the exact columns specified, leaving unwritten columns with their previous pixel values. This causes "ghosting" when content shrinks (shorter text, fewer digits). The following patterns exist to prevent residue:

- **`OLED_ClearPage(y)`**: clears an entire 128-column page (8 pixel rows). Used on alarm→normal transition to clear page 7 (bottom halves of 8×16 alarm text).
- **`OLED_FillBytes(x, y, len, 0x00)`**: clears `len` columns starting at `(x, y)`. Used in `OLED_ShowEnv()` to clear every gap between fixed field positions and the variable-width integer fields — this handles both digit-count changes and DHT11 error-text recovery.
- **Fan status padding**: `"FAN:ON "` includes a trailing space so both ON/OFF strings occupy exactly 7 characters (42 pixels), preventing residue on state change.

When adding new OLED content, always consider: does the new content fully cover the worst-case extent of the previous content? If not, use `OLED_FillBytes` to clear the difference.

## Peripheral Clock Speeds

```
HSE 25MHz → PLL (M=25,N=336,P=2) → SYSCLK=168MHz
AHB1 = 168MHz (GPIO, DMA2)
APB1 = 42MHz  → TIM3_CLK = 2×APB1 = 84MHz
APB2 = 84MHz  → ADC_CLK = APB2/4 = 21MHz
```

FPU (CP10/CP11) is enabled automatically by `SystemInit()` in the standard library startup sequence.

## Version Pinning

- **ST firmware**: STM32F4xx_StdPeriph_Driver V1.8.0 (vendored in `Libraries/`)
- **ARM Compiler**: V5.06 update 7 — do NOT use AC6/ARMClang, the code uses ARMCC-specific syntax (e.g. `__IO`, `__attribute__` patterns from CMSIS)
- **IDE**: Keil MDK 5.38
