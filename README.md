# EMS 环境监测系统 — 野火霸天虎 STM32F407 开发板

---

## 程序简介

- **工程名称**：EMS 环境监测系统
- **实验平台**: 野火 STM32 F407 霸天虎开发板
- **MDK版本**：5.38
- **ST固件库版本**：STM32F4xx_StdPeriph_Driver V1.8.0
- **ARM编译器**：ARMCC V5.06 update 7
- **调试器**：Fire CMSIS-DAP (SWD)

---

## 功能简介

系统实时采集 **DHT11 温湿度、MQ135 空气质量、MQ2 烟雾浓度**，通过 **OLED 屏幕** 滚动显示全部传感器数据。当温度过高或空气质量恶化时，自动启动 **TB6612FNG 驱动的直流风扇** 进行通风，并在 OLED 上显示报警图标，同时 RGB LED 由绿色变为红色。

**核心功能**：
- 每 2 秒采集一次全部传感器数据
- OLED 实时显示温度、湿度、气体浓度
- 温度 > 30°C 或气体浓度超标 → 自动启动风扇 (60% 占空比)
- 传感器故障时显示 `DHT11: --` 提示
- 串口同步输出所有数据供调试

**学习目的**：掌握多传感器协同采集、I2C OLED 显示、ADC DMA 多通道扫描、DWT 微秒延时、TIM PWM 电机控制、DHT11 单总线通信等综合应用。

---

## 开发环境准备

### 1. Keil MDK-ARM 安装

- 下载并安装 Keil MDK-ARM V5.x
- 通过 Pack Installer 安装器件包 `Keil.STM32F4xx_DFP` (2.5.0+)
- 安装 ARM Compiler 5 (ARMCC V5.06) — 在 Keil 官网下载 `ARMCompiler_506-Windows`

### 2. 调试器驱动

- 安装 Fire CMSIS-DAP 调试器驱动（野火 DAP 仿真器）
- 确认设备管理器中能识别 CMSIS-DAP 设备

### 3. 打开工程

用 Keil MDK 打开：

```
Project\RVMDK（uv5）\EMS_Project.uvprojx
```

### 4. 编译与下载

1. Options → Target → 确认器件为 `STM32F407ZGTx`
2. Options → C/C++ → Define 包含 `USE_STDPERIPH_DRIVER,STM32F40_41xxx`
3. Options → Target → 勾选 `Use MicroLIB`
4. F7 编译 → F8 下载 → 复位运行

### 5. 查看输出

- 串口助手选择对应 COM 口，设置 **115200-8-N-1**
- 复位后打印系统信息，开始循环输出传感器数据
- OLED 同时显示所有数据

---

## 引脚分配

霸天虎 F407 开发板使用 **25MHz** 外部高速晶振，所有 GPIO 时钟挂在 AHB1 总线。

```
                      霸天虎 STM32F407ZGTx
            ┌──────────────────────────────────────┐
      DHT11 │ PA6  ○ (DATA)                ○  PA9  │ USART1 TX
            │                              ○ PA10  │ USART1 RX
      MQ135 │ PA4  ○ (AO/ADC1_CH4)                 │
            │ PC4  ○ (DO/GPIO输入)          ○  PB6 │ OLED SCL (I2C1)
      MQ2   │ PA1  ○ (AO/ADC1_CH1)                 │
            │ PC5  ○ (DO/GPIO输入)          ○  PB7 │ OLED SDA (I2C1)
     TB6612 │ PB0  ○ (PWMA/TIM3_CH3)               │
    (风扇)  │ PC0  ○ (AIN1)                 ○  PF6 │ LED R
           │ PC1   ○ (AIN2)                 ○  PF7 │ LED G
           │ PC2   ○ (STBY/高电平使能)       ○  PF8 │ LED B
           └───────────────────────────────────────┘
```

### 详细接线表

| 模块 | 模块引脚 | STM32引脚 | 功能 | 注意事项 |
|------|---------|----------|------|---------|
| **DHT11** | VCC | 3.3V | 电源 | 也可 5V |
| | GND | GND | | |
| | DATA | **PA6** | 单总线数据 | 外接 4.7kΩ 上拉到 3.3V |
| **MQ135** | VCC | **5V** | 加热电源 | 模块有 LDO, 不可接 3.3V |
| | GND | GND | | |
| | AO | **PA4** | 模拟输出 0-5V | ADC1_CH4, 建议加分压电阻 |
| | DO | **PC4** | 数字阈值 | 低电平=超标, 电位器调阈值 |
| **MQ2** | VCC | **5V** | 加热电源 | 同上 |
| | GND | GND | | |
| | AO | **PA1** | 模拟输出 0-5V | ADC1_CH1 |
| | DO | **PC5** | 数字阈值 | 低电平=超标 |
| **OLED** | VCC | 3.3V | | 0.96寸 I2C SSD1306 |
| | GND | GND | | |
| | SCL | **PB6** | I2C1 时钟 | 外接 4.7kΩ 上拉到 3.3V |
| | SDA | **PB7** | I2C1 数据 | 外接 4.7kΩ 上拉到 3.3V |
| **TB6612** | VM | 5~12V | 电机电源 | 根据风扇规格选择 |
| (风扇) | VCC | 5V | 逻辑电源 | |
| | GND | GND | | 必须与 STM32 共地 |
| | PWMA | **PB0** | PWM 调速 | TIM3_CH3, 10kHz, 0-100% |
| | AIN1 | **PC0** | 方向控制 | |
| | AIN2 | **PC1** | 方向控制 | |
| | STBY | **PC2** | 使能 | 高电平=正常工作 |
| **USART1** | TX | **PA9** | 串口发送 | 接 CH340 USB-TTL 的 RX |
| | RX | **PA10** | 串口接收 | 接 CH340 USB-TTL 的 TX |

### 外接元件说明

| 元件 | 位置 | 用途 |
|------|------|------|
| 4.7kΩ 电阻 ×3 | DHT11 DATA, OLED SCL, OLED SDA | I2C/单总线上拉 |
| 10kΩ + 20kΩ 分压 | MQ135 AO, MQ2 AO → GND | 5V→3.3V 电平匹配 (可选) |
| 直流风扇 | TB6612 电机输出 A01/A02 | 通风风扇 |

---

## 时钟

- **外部高速晶振 (HSE)**：25MHz
- **RTC 晶振**：32.768KHz

**各总线运行时钟**：

```
HSE (25MHz)
  → PLL (M=25, N=336, P=2, Q=7)
    → SYSCLK = AHB1 = 168MHz  (GPIO, DMA)
    → APB1   =  42MHz → TIM3_CLK = 2×APB1 = 84MHz (PWM 定时器)
    → APB2   =  84MHz → ADC_CLK  = APB2/4  = 21MHz (模拟采样)
```

**浮点运算单元 (FPU)**：由 `SystemInit()` 自动使能 (CP10/CP11 完全访问)。

---

## 系统工作流程

```text
main()
  │
  ├── DWT_Delay_Init()           // 使能 CYCCNT 微秒延时
  ├── USART_Config()             // PA9/PA10, 115200-8-N-1
  ├── LED_GPIO_Config()          // PF6/7/8, 推挽输出
  ├── DHT11_GPIO_Config()        // PA6, 初始推挽输出高
  ├── MQ135_Init()               // PA4 模拟输入 + ADC1 DMA 双通道配置
  ├── MQ2_Init()                 // PA1 模拟输入 (ADC 由 MQ135 统一配)
  ├── IIC_GPIO_Config()          // PB6/PB7, I2C1 400kHz 开漏
  ├── OLED_Init()                // SSD1306 全初始化序列
  ├── TB6612_Init()              // PB0 TIM3_CH3 PWM + PC0/1/2 GPIO
  │
  └── while(1) 每 2 秒循环:
        ├── Read_DHT11()         // 单总线读取, 校验和验证
        ├── MQ135_Get_PPM()      // 从 DMA 缓冲读 ADC, 幂函数计算
        ├── MQ2_Get_PPM()        // 同上
        ├── GPIO_ReadInputDataBit() 查 DO 引脚状态
        ├── CheckAlarm()         // PPM 阈值 + DO(仅传感器在线时采信)
        ├── 超限 → TB6612_Forward() + SetSpeed(60%)
        │        LED_RED, OLED 显示 "** ALARM **"
        └── 正常 → TB6612_Stop(), LED_GREEN, OLED 显示实时浓度
```

---

## 报警逻辑

| 检测项 | 阈值 | 定义位置 | 动作 |
|--------|------|---------|------|
| 温度 | > 30°C | `TEMP_ALARM` | 风扇 ON |
| MQ135 PPM | > 500 | `MQ135_ALARM` | 风扇 ON |
| MQ2 PPM | > 5000 | `MQ2_ALARM` | 风扇 ON |
| MQ135 DO | = LOW **且** PPM > 1.0 | 硬件阈值 + `SENSOR_ONLINE_PPM` | 风扇 ON |
| MQ2 DO | = LOW **且** PPM > 1.0 | 硬件阈值 + `SENSOR_ONLINE_PPM` | 风扇 ON |

> **DO 判定的前提**：DO 引脚低有效，但只有在传感器**在线**（PPM > `SENSOR_ONLINE_PPM`）时才采信。传感器未供电或未接线时 ADC 读到 0、DO 同时被拉低，若不加此限制会误启动风扇。此时 `MQ135_Get_PPM()` / `MQ2_Get_PPM()` 直接返回 0（避免除零得到 inf/NaN），串口打印 `[WARN] ... offline (ADC=0), check power/wiring`。

**修改阈值**：打开 `User\main.c`，修改顶部宏定义：

```c
#define TEMP_ALARM        30      // 温度阈值 (°C)
#define MQ135_ALARM       500     // MQ135 阈值 (ppm)
#define MQ2_ALARM         5000    // MQ2 阈值 (ppm)
#define FAN_SPEED         60      // 风扇转速 (0-100%)
#define SENSOR_ONLINE_PPM 1.0f    // 传感器在线判定下限 (ppm)
```

---

## 项目结构

```text
EMS_Project/
├── README.md                       // 本文件
├── User/
│   ├── main.c                      // 主程序 (传感器采集+报警+显示循环)
│   ├── stm32f4xx_conf.h            // 外设头文件 (ADC/DMA/I2C/TIM/USART...)
│   ├── stm32f4xx_it.c/h            // 中断服务函数
│   │
│   ├── dht11/                      // DHT11 温湿度传感器
│   │   ├── bsp_dht11.c             //   单总线协议 (20ms起始+读5字节+校验)
│   │   └── bsp_dht11.h             //   引脚: PA6, DHT11_Data_TypeDef 结构体
│   │
│   ├── mq135/                      // MQ135 空气质量传感器
│   │   ├── bsp_mq135.c             //   ADC1 DMA 双通道配置 (含 MQ2)
│   │   └── bsp_mq135.h             //   PA4(CH4) + 传感器常数 + DMA 宏
│   │
│   ├── mq2/                        // MQ2 烟雾传感器
│   │   ├── bsp_mq2.c               //   PPM 计算 (ADC 由 MQ135 管理)
│   │   └── bsp_mq2.h               //   PA1(CH1) + MQ2 传感器常数
│   │
│   ├── oled/                       // OLED 0.96" I2C SSD1306
│   │   ├── bsp_oled.c              //   全初始化序列 + ASCII/中文/BMP 渲染
│   │   ├── bsp_oled.h              //   PB6(SCL)/PB7(SDA), I2C 地址 0x78
│   │   └── bsp_oled_codetab.h      //   字库: F6x8 + F8X16 + F16x16
│   │
│   ├── iic/                        // I2C 总线驱动
│   │   ├── bsp_iic_debug.c         //   硬件 I2C1 400kHz + 软件 I2C 兼容
│   │   └── bsp_iic_debug.h         //   IIC_SELECT 宏切换双模式
│   │
│   ├── tb6612/                     // TB6612FNG 电机驱动
│   │   ├── bsp_tb6612.c            //   TIM3_CH3 PWM 10kHz + 方向控制
│   │   └── bsp_tb6612.h            //   PB0(PWM) PC0/1(AIN) PC2(STBY)
│   │
│   ├── usart/                      // USART1 调试串口
│   │   ├── bsp_usart.c             //   115200-8-N-1, fputc/fgetc 重定向
│   │   └── bsp_usart.h
│   │
│   ├── led/                        // 板载 RGB LED
│   │   ├── bsp_led.c               //   野火 F407 官方驱动
│   │   └── bsp_led.h               //   PF6(R)/PF7(G)/PF8(B), 颜色宏
│   │
│   ├── key/                        // 按键扫描
│   │   ├── bsp_key.c
│   │   └── bsp_key.h               //   PA0(KEY1) PC13(KEY2)
│   │
│   └── dwt_delay/                  // 微秒延时
│       ├── core_delay.c/h          //   DWT CYCCNT 精确延时 (jiejieTop)
│       └── bsp_dwt_delay.c/h       //   兼容包装 (DWT_Delay_ms → CPU_TS_Tmr_Delay)
│
├── Libraries/                      // STM32F4xx_StdPeriph_Driver V1.8.0
│   ├── CMSIS/
│   │   ├── Device/ST/STM32F4xx/
│   │   │   ├── Include/            //    stm32f4xx.h, system_stm32f4xx.h
│   │   │   └── Source/Templates/
│   │   │       ├── arm/startup_stm32f40xx.s  // 启动文件
│   │   │       └── system_stm32f4xx.c        // 时钟初始化
│   │   └── Include/                //    core_cm4.h 等 CMSIS 核心头文件
│   └── STM32F4xx_StdPeriph_Driver/
│       ├── inc/                    //    外设头文件 (.h)
│       └── src/                    //    外设源文件 (.c)
│
├── Project/RVMDK（uv5）/           // Keil MDK 项目
│   ├── EMS_Project.uvprojx         //   项目文件 (编译选项 + 文件列表)
│   ├── EMS_Project.uvoptx          //   项目选项 (编辑器/调试器状态)
│   └── DebugConfig/                //   调试 MCU 配置
│
├── Listing/                        // 编译清单 (.map, .lst)
└── Output/                         // 编译输出 (.axf, .hex)
```

---

## Keil 项目配置说明

### 项目分组

| 组名 | 文件 | 说明 |
|------|------|------|
| **STARTUP** | `startup_stm32f40xx.s` | 启动文件，Cortex-M4 向量表 |
| **CMSIS** | `system_stm32f4xx.c` | 系统时钟初始化 168MHz |
| **STM32F4xx_StdPeriph_Driver** | 38 个外设源文件 | 标准外设库 |
| **USER** | 13 个用户源文件 | main.c + 11 个 bsp 模块 + stm32f4xx_it.c |
| **DOC** | 必读说明.txt | 工程说明文档 |

### 关键编译选项

| 选项 | 值 | 说明 |
|------|-----|------|
| Device | STM32F407ZGTx | Cortex-M4, 1024KB Flash, 192KB SRAM |
| Define | `USE_STDPERIPH_DRIVER,STM32F40_41xxx` | 使能标准外设库 + 选择芯片型号 |
| IncludePath | User; Libraries\...\inc; Libraries\CMSIS\Include; Libraries\...\STM32F4xx\Include | 4 个包含路径 |
| Optimization | Level 2 (-O2) | |
| MicroLIB | Enabled | 精简 C 库, 减少代码体积 |
| Hex Output | Enabled | 生成 .hex 烧录文件 |
| ARM Compiler | V5.06 update 7 | |

---

## 各模块驱动详解

### DHT11 温湿度传感器 (bsp_dht11)

> **官方参考**：`DHT11\2-配套例程_接线说明\F407_霸天虎开发板\串口显示温湿度\`

- **引脚**：PA6 (适配自官方例程的 PE3)
- **通信协议**：单总线 (One-Wire)
- **时序**：
  1. 主机拉低 20ms (起始信号)
  2. 主机拉高 30μs, 切换为输入模式
  3. 从机拉低 80μs + 拉高 80μs (响应信号)
  4. 读取 5 字节 (湿度整数+小数+温度整数+小数+校验和)
- **延时依赖**：`core_delay.c` 提供的 `CPU_TS_Tmr_Delay_US()` (DWT CYCCNT)
- **数据结构**：`DHT11_Data_TypeDef { humi_int, humi_deci, temp_int, temp_deci, check_sum }`

### MQ135 + MQ2 气体传感器 (bsp_mq135 / bsp_mq2)

> **官方参考**：`f407\ADC-独立模式-多通道DMA读取数据\`

- **ADC 配置**：ADC1 双通道 DMA 扫描模式
  - CH1 (PA1) = MQ2, CH4 (PA4) = MQ135
  - DMA2_Stream0_Channel0, 循环模式, 半字传输
  - ADC 时钟 = PCLK2/4 = 21MHz
- **PPM 计算公式**：

  ```
  Vout = ADC_Value / 4096 × Vc
  Rs   = (Vc - Vout) × RL / Vout
  PPM  = A × (Rs/R0)^B
  ```

- **传感器常数** (在 `bsp_mq135.h` / `bsp_mq2.h` 中)：
  - MQ135: RL=1kΩ, R0=2kΩ, A=4.17, B=-2.28
  - MQ2:   RL=10kΩ, R0=20kΩ, A=43.03, B=-1.66

### OLED SSD1306 I2C 显示屏 (bsp_oled + bsp_iic)

> **官方参考**：`OLED_I2C_0.96\2-配套程序_接线说明\F407_霸天虎开发板\标准库\`

- **I2C 驱动**：I2C1, PB6(SCL)/PB7(SDA), 400kHz, 7-bit 地址
  - 硬件 I2C (默认)：`IIC_SELECT 1`
  - 软件 I2C (兼容)：`IIC_SELECT 0`, GPIO 模拟时序
- **SSD1306 初始化**：完整的 20+ 条命令序列
  - 电荷泵使能, 页寻址模式, COM 扫描方向, 段重映射, 对比度设置
- **字库支持**：
  - F6x8: 95 个 ASCII 字符 (6×8 像素)
  - F8X16: 95 个 ASCII 字符 (8×16 像素)
  - F16x16: 4 个中文字符 ("野火科技")

### TB6612FNG 电机驱动 (bsp_tb6612)

- **PWM 配置**：TIM3_CH3 (PB0), 10kHz
  - PSC = 84, ARR = 100
  - 84MHz / (84 × 100) = 10kHz
- **控制逻辑**：
  - `TB6612_Forward()`: AIN1=0, AIN2=1 → 正转
  - `TB6612_Backward()`: AIN1=1, AIN2=0 → 反转
  - `TB6612_Stop()`: AIN1=0, AIN2=0 → 短路制动
  - `TB6612_SetSpeed(duty)`: 直接写 `TIM3->CCR3`

### DWT 微秒延时 (core_delay)

> **官方参考**：`DHT11\2-配套例程_接线说明\F407_霸天虎开发板\串口显示温湿度\User\dwt_delay\`

- **原理**：读取 Cortex-M4 的 DWT CYCCNT 寄存器 (每 CPU 周期+1)
- **初始化**：`CPU_TS_TmrInit()` 使能 DEMCR.TRCENA + DWT_CTRL.CYCCNTENA
- **延时**：`CPU_TS_Tmr_Delay_US(us)` 换算为 CPU 周期数，忙等待
- **兼容包装**：`bsp_dwt_delay.h` 提供 `DWT_Delay_ms(ms)` 简单调用

---

## 参考资源

### 本工程参照的官方 F407 例程

| 例程 | 路径 | 用途 |
|------|------|------|
| GPIO 按键检测 | `f407\13-GPIO输入—按键检测\` | LED/Key 驱动 |
| USART 串口控制 | `f407\USART—控制RGB灯的亮灭\` | USART printf 重定向 |
| ADC 独立模式 DMA | `f407\ADC-独立模式-多通道DMA读取数据\` | MQ135/MQ2 ADC 采集 |
| I2C EEPROM 读写 | `f407\24-I2C—读写EEPROM\` | I2C1 初始化参考 |
| TIM 基本定时器 | `f407\32-TIM—基本定时器定时\` | 定时器配置参考 |
| DHT11 温湿度 | `DHT11\2-配套例程_接线说明\F407_霸天虎开发板\` | DHT11 单总线 + DWT 延时 |
| OLED I2C 显示 | `OLED_I2C_0.96\2-配套程序_接线说明\F407_霸天虎开发板\标准库\` | OLED SSD1306 + I2C 驱动 |

### 模块数据手册

| 模块 | 路径 |
|------|------|
| DHT11 模块 | `DHT11\1-硬件资料_参考资料_模块手册\` |
| OLED 0.96" | `OLED_I2C_0.96\1-硬件资料_参考资料_模块手册\` |
| MQ135 | `MQ135 Air Quality\1-硬件资料_参考资料_模块手册\` |
| MQ2 | `MQ2 Smog\1-硬件资料_参考资料_模块手册\` |
| TB6612FNG | `TB6612FNG\1-硬件资料_参考资料_模块手册\` |

### STM32F4 官方库

| 资源 | 路径 |
|------|------|
| 官方 DSP 标准外设库 | `STM32 Reference manual\STM32F4xx_DSP_StdPeriph_Lib_V1.8.0\` |
| F407 模板 (MDK-ARM) | `...\Project\STM32F4xx_StdPeriph_Templates\MDK-ARM\` |

---

## 故障排查

| 现象 | 可能原因 | 解决方法 |
|------|---------|---------|
| 串口无输出 | PA9/PA10 接线错误, 波特率不匹配 | 确认接线, 串口助手设为 115200-8-N-1 |
| LED 不亮 | 端口未初始化 | 确认 `LED_GPIO_Config()` 已调用 |
| DHT11 读取失败 | 上拉电阻缺失, 时序受干扰 | 检查 PA6 外接 4.7kΩ 上拉, 缩短杜邦线 |
| MQ 传感器读数全 0 | 传感器未供电或未预热 | 检查 5V 供电, 等待 1-2 分钟预热 |
| MQ PPM 值恒定为 0 | ADC DMA 未启动 | 确认 `MQ135_Init()` 在 main 中已调用 |
| 串口打印 `[WARN] ... offline (ADC=0)` | 传感器未供电或未接线, ADC 读到 0 | 检查 5V 供电与 AO 接线; 此告警**不会**启动风扇 |
| OLED 无显示 | I2C 无应答, 上拉缺失 | 检查 PB6/PB7 接线 + 外接 4.7kΩ 上拉 |
| OLED 花屏 | SSD1306 初始化序列有误 | 确认 `OLED_Init()` 完整调用 |
| 风扇不转 | STBY 未拉高, 电机电源不足 | 检查 PC2=高电平, VM 供电电压 5~12V |
| 风扇转速异常 | PWM 频率/占空比配置错误 | 确认 TIM3 PSC=84, ARR=100 |
| 编译报错 #53/#127 | LED 宏在 if/else 中无括号 | 已修复: 所有 LED 宏使用均加 `{ }` |
| 编译报错 #147 | 字体数组 const 不一致 | 已修复: F16x16 加 const 前缀 |
| 链接报错 (undefined) | 缺少 MicroLIB | `Options → Target → Use MicroLIB` 勾选 |

---

## 版本

- **程序版本**：1.3
- **发布日期**：2026-10
- **版本更新说明**：
  - V1.0 (2026-05): 首次发布, 6 种演示模式验证各模块
  - V1.1 (2026-06): 正式实现实时环境监测系统, 集成 DHT11/MQ135/MQ2/OLED/TB6612 自动报警
  - V1.1.1 (2026-06): 更新 README
  - V1.2 (2026-06): 补充工程文档、架构图与引脚图, 修复 OLED 页缓冲区残留
  - V1.3 (2026-10): 修复传感器离线时的除零与误报警, DO 判定改为仅在传感器在线时采信, 新增 ADC=0 离线告警; 不再跟踪 Keil 用户界面状态文件 (`*.uvguix.*`)

---

## 联系我们

- 野火官网: <https://embedfire.com>
- 野火论坛: <http://www.firebbs.cn>
- 野火商城: <https://yehuosm.tmall.com/>
- 野火资料下载中心: <http://doc.embedfire.com/products/link>
