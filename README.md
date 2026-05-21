# EMS 综合项目 — 霸天虎 STM32F407 开发板

11111

环境监测系统 (Environment Monitor System)，集成 **DHT11 温湿度、MQ135 空气质量、MQ2 烟雾、OLED 显示屏、TB6612FNG 电机驱动** 五大模块。

## 开发环境

| 工具 | 版本 |
|------|------|
| Keil MDK-ARM | 5.x (测试于 V5.38) |
| 器件包 | Keil.STM32F4xx_DFP.2.5.0+ |
| 固件库 | STM32F4xx_StdPeriph_Driver V1.8.0 |
| ARM 编译器 | ARMCC V5.06 |
| 调试器 | Fire CMSIS-DAP (SWD) |

## 快速开始

1. 用 Keil MDK 打开 `Project\RVMDK（uv5）\EMS_Project.uvprojx`
2. 确认 `Options → Target` 器件为 `STM32F407ZGTx`
3. 确认 `Options → C/C++ → Define` 包含 `USE_STDPERIPH_DRIVER,STM32F40_41xxx`
4. 编译 (F7) → 下载 (F8) → 复位运行
5. 打开串口助手 (115200-8-N-1)，按 KEY1/KEY2 交互

## 引脚分配总表

```
                    霸天虎 STM32F407ZGTx
         ┌──────────────────────────────────────┐
         │                                      │
    KEY1 │ PA0  ○                        ○  PA9 │ USART1 TX
    KEY2 │ PC13 ○                        ○ PA10 │ USART1 RX
         │                                      │
  DHT11  │ PA6  ○                        ○  PB6 │ OLED SCL (I2C1)
         │      ○                        ○  PB7 │ OLED SDA (I2C1)
  MQ135  │ PA4  ○ (AO)                           │
         │ PC4  ○ (DO)                    ○  PF6 │ LED R
  MQ2    │ PA1  ○ (AO)                           │
         │ PC5  ○ (DO)                    ○  PF7 │ LED G
         │                                ○  PF8 │ LED B
 TB6612  │ PB0  ○ (PWMA)                          │
         │ PC0  ○ (AIN1)                          │
         │ PC1  ○ (AIN2)                          │
         │ PC2  ○ (STBY)                          │
         │                                      │
         └──────────────────────────────────────┘
```

### 详细接线表

| 模块 | 模块引脚 | STM32 引脚 | 说明 |
|------|---------|-----------|------|
| **MQ135** | VCC | **5V** | 必须 5V 供电, 模块有 3.3V LDO |
| | GND | GND | |
| | AO | **PA4** | ADC1_CH4, 模拟输出 0-5V |
| | DO | **PC4** | 数字阈值输出 (低电平=超标) |
| **MQ2** | VCC | **5V** | 必须 5V 供电 |
| | GND | GND | |
| | AO | **PA1** | ADC1_CH1, 模拟输出 0-5V |
| | DO | **PC5** | 数字阈值输出 |
| **OLED** | VCC | **3.3V** | 0.96寸 I2C SSD1306 |
| | GND | GND | |
| | SCL | **PB6** | I2C1 时钟线, 外接 4.7kΩ 上拉到 3.3V |
| | SDA | **PB7** | I2C1 数据线, 外接 4.7kΩ 上拉到 3.3V |
| **DHT11** | VCC | **3.3V** | 也可 5V, 但 DATA 上拉需匹配 |
| | GND | GND | |
| | DATA | **PA6** | 外接 4.7kΩ 上拉到 3.3V |
| **TB6612** | VCC | **5V** | 电机驱动电源 |
| | GND | GND | 与 STM32 共地 |
| | PWMA | **PB0** | PWM 调速 (TIM3_CH3, 10kHz) |
| | AIN1 | **PC0** | 方向控制 |
| | AIN2 | **PC1** | 方向控制 |
| | STBY | **PC2** | 高电平 = 使能 |
| **USART1** | TX | **PA9** | 接 CH340 USB 转串口 RX |
| | RX | **PA10** | 接 CH340 USB 转串口 TX |
| **板载** | KEY1 | **PA0** | 板载按键, 按下为高电平 |
| | KEY2 | **PC13** | 板载按键, 按下为高电平 |
| | LED_R | **PF6** | 板载 RGB LED 红色, 低电平亮 |
| | LED_G | **PF7** | 板载 RGB LED 绿色, 低电平亮 |
| | LED_B | **PF8** | 板载 RGB LED 蓝色, 低电平亮 |

> **重要提示**:
> - MQ135/MQ2 的 AO 输出是 **0-5V**, 直接接 F407 的 3.3V ADC 会饱和。建议在 AO 与 GND 之间接 10kΩ+20kΩ 分压 (VO ÷ 3 × 2 ≈ ADC 电压), 或修改代码中 `MQ135_VC` / `MQ2_VC` 为 3.3V
> - OLED 和 DHT11 的 I2C/单总线引脚**必须外接 4.7kΩ 上拉电阻到 3.3V**
> - TB6612FNG 电机电源 (VM) 根据电机规格选择 5~12V, 逻辑电源 (VCC) 接 5V
> - 所有模块 GND 必须与 STM32 GND 共地

## 项目结构

```
EMS_Project/
├── README.md
├── User/
│   ├── main.c                  ← 主程序 (6 种测试模式)
│   ├── stm32f4xx_conf.h        ← 外设头文件配置
│   ├── stm32f4xx_it.c/h        ← 中断服务函数
│   ├── usart/bsp_usart.c/.h    ← USART1 驱动 (printf 就绪)
│   ├── led/bsp_led.c/.h        ← 板载 RGB LED 驱动
│   ├── key/bsp_key.c/.h        ← 按键扫描驱动
│   ├── dwt_delay/              ← 微秒延时 (Cortex-M4 DWT)
│   ├── mq135/bsp_mq135.c/.h    ← MQ135 传感器 (含 ADC1 DMA 双通道)
│   ├── mq2/bsp_mq2.c/.h        ← MQ2 传感器
│   ├── oled/bsp_oled.c/.h      ← OLED SSD1306 I2C 驱动
│   ├── dht11/bsp_dht11.c/.h    ← DHT11 单总线驱动
│   └── tb6612/bsp_tb6612.c/.h  ← TB6612FNG 电机驱动
├── Libraries/                  ← STM32F4xx_StdPeriph_Driver V1.8.0
│   ├── CMSIS/
│   └── STM32F4xx_StdPeriph_Driver/
├── Project/RVMDK（uv5）/       ← Keil MDK 项目文件
├── Listing/                    ← 编译清单
└── Output/                     ← 编译输出 (hex/bin/axf)
```

## 运行模式说明

上电后串口打印系统信息, 默认进入 **Mode 0 (自检)**。操作按键切换:

| 模式 | 名称 | 功能 | 触发方式 |
|------|------|------|---------|
| **Mode 0** | 系统自检 | LED 三色流水灯 + 打印系统信息 | KEY2: 重新打印 |
| **Mode 1** | 气体传感器 | MQ135+MQ2 ADC 读数、PPM、DO 状态 | 自动刷新, KEY2 打印一次 |
| **Mode 2** | OLED 测试 | I2C 总线应答检测 + 发送 Display ON | KEY2: 发送数据 |
| **Mode 3** | DHT11 | 单总线读取温湿度 | KEY2: 触发一次 |
| **Mode 4** | TB6612FNG | 电机调速 (15% 步进) | KEY2: 加速 |
| **Mode 5** | 全模块循环 | 每 2 秒轮流展示各传感器 | 自动循环 |

- **KEY1 (PA0)**: 短按切换模式 (0→1→2→3→4→5→0), LED 颜色跟随模式变化
- **KEY2 (PC13)**: 在当前模式触发动作

## 各模块实现状态

| 模块 | 硬件初始化 | 通信协议 | 数据读取 | 备注 |
|------|:---:|:---:|:---:|------|
| USART | ✅ | ✅ | ✅ | 野火官方 F407 驱动, printf 就绪 |
| LED | ✅ | — | ✅ | 野火官方 F407 驱动 |
| Key | ✅ | ✅ | ✅ | 野火官方 F407 驱动, 高电平有效 |
| DWT_Delay | ✅ | ✅ | ✅ | Cortex-M4 CYCCNT, us/ms 延时 |
| MQ135 | ✅ | ✅ | ✅ | ADC1 DMA 双通道, PPM 公式就绪 |
| MQ2 | ✅ | ✅ | ✅ | 与 MQ135 共用 ADC1 DMA |
| OLED | ✅ | ✅ | ⬜ | I2C1 硬件就绪, SSD1306 初始化序列待填充 |
| DHT11 | ✅ | ⬜ | ⬜ | GPIO 输入/输出切换框架就绪, 单总线时序待填充 |
| TB6612FNG | ✅ | ⬜ | ⬜ | GPIO 控制就绪, TIM3 PWM 配置待填充 |

## 移植指南

### 添加新传感器驱动

1. 在 `User/` 下创建模块文件夹 (如 `User/xxx/`)
2. 编写 `bsp_xxx.h` (引脚宏定义 + 函数声明) 和 `bsp_xxx.c`
3. 在 `main.c` 中 `#include "./xxx/bsp_xxx.h"`
4. 在 Keil 项目 USER 组中添加 `bsp_xxx.c`
5. 在 `main()` 中调用初始化函数

### ADC 使用说明

MQ135 和 MQ2 共用 ADC1 双通道 DMA 扫描模式:

```c
// 在 bsp_mq135.h 中定义
extern __IO uint16_t MQ_ADC_Values[2];
// MQ_ADC_Values[0] = ADC1_CH1 (MQ2 AO = PA1)
// MQ_ADC_Values[1] = ADC1_CH4 (MQ135 AO = PA4)
```

若要新增 ADC 通道:
1. 在 `bsp_mq135.h` 修改 `MQ_ADC_CHANNEL_COUNT`
2. 在 `bsp_mq135.c` 的 `MQ_ADC_GPIO_Config()` 添加 GPIO 初始化
3. 在 `MQ_ADC_Mode_Config()` 添加 `ADC_RegularChannelConfig()` 调用
4. 更新 DMA `BufferSize`

### I2C 使用说明

OLED 使用 I2C1 (PB6/PB7), 400kHz。如需改为 PB8/PB9 (与 EEPROM 例程一致):

```c
// bsp_oled.h 中修改
#define OLED_SCL_PIN      GPIO_Pin_8
#define OLED_SCL_SOURCE   GPIO_PinSource8
#define OLED_SDA_PIN      GPIO_Pin_9
#define OLED_SDA_SOURCE   GPIO_PinSource9
```

### PWM 使用说明

TB6612FNG 使用 TIM3_CH3 (PB0):

```
F407 时钟树:
  HSE (8MHz) → PLL (×336/8/2) → SYSCLK = 168 MHz
    → AHB1 = 168 MHz (GPIO 时钟)
    → APB1 = 42 MHz  → TIM3_CLK = 2×APB1 = 84 MHz
    → APB2 = 84 MHz  → ADC_CLK = APB2/4 = 21 MHz

PWM 频率计算:  84 MHz / (PSC × ARR) = 84 MHz / (84 × 100) = 10 kHz
占空比:        CCR3 / ARR × 100%
```

## 参考资源

| 资源 | 路径 |
|------|------|
| 官方 F407 模板 | `STM32 Reference manual\STM32F4xx_DSP_StdPeriph_Lib_V1.8.0\Project\STM32F4xx_StdPeriph_Templates\` |
| I2C EEPROM 例程 | `f407\24-I2C—读写EEPROM\` |
| ADC 中断例程 | `f407\ADC-独立模式-中断读取数据\` |
| ADC DMA 多通道 | `f407\ADC-独立模式-多通道DMA读取数据\` |
| GPIO/LED 例程 | `f407\12-GPIO输出—使用固件库点亮LED灯\` |
| USART 例程 | `f407\USART—控制RGB灯的亮灭\` |
| TIM 定时器 | `f407\32-TIM—基本定时器定时\` |
| DHT11 模块手册 | `DHT11\1-硬件资料_参考资料_模块手册\` |
| OLED 模块手册 | `OLED_I2C_0.96\1-硬件资料_参考资料_模块手册\` |
| MQ135 模块手册 | `MQ135 Air Quality\1-硬件资料_参考资料_模块手册\` |
| MQ2 模块手册 | `MQ2 Smog\1-硬件资料_参考资料_模块手册\` |
| TB6612FNG 模块手册 | `TB6612FNG\1-硬件资料_参考资料_模块手册\` |

## 按键/模式操作

| 按键 | 功能 |
|------|------|
| KEY1 (PA0) | 短按: 模式+1 (0→1→2→3→4→5→0) |
| KEY2 (PC13) | 短按: 当前模式触发动作 |

## 故障排查

| 现象 | 可能原因 | 解决方法 |
|------|---------|---------|
| 串口无输出 | USART1 引脚错误 | 检查 PA9/PA10 接线, 115200-8-N-1 |
| LED 不亮 | PF 端口未初始化 | 确认 LED_Init() 已调用 |
| MQ 传感器读数全 0 | ADC 未配置或传感器未供电 | 检查 5V 供电, PA1/PA4 接线 |
| MQ 传感器 PPM 值异常 | 传感器未预热 | 上电等待 1-2 分钟 |
| OLED 无显示 | I2C 无应答 | 检查 PB6/PB7 接线 + 4.7kΩ 上拉 |
| DHT11 读取失败 | 时序问题或上拉缺失 | 检查 PA6 外接 4.7kΩ 上拉 |
| 电机不转 | STBY 未拉高 / 供电不足 | 检查 PC2=高, VM 供电电压 |
| 编译错误 | 缺少器件包 | 安装 Keil.STM32F4xx_DFP.2.5.0+ |
