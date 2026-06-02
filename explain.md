# EMS 环境监测系统 — 项目详解

---

## 一、项目概述

本系统基于 **STM32F407ZGTx（Cortex-M4，168MHz）** 微控制器，运行在野火"霸天虎"开发板上。系统实时采集 **温湿度、空气质量、烟雾浓度** 三类环境数据，通过 **OLED 屏幕** 滚动显示，并在异常时自动启动 **直流风扇** 通风并触发声光报警。

**核心传感器**：DHT11（温湿度）、MQ135（空气质量）、MQ2（烟雾浓度）
**执行器**：TB6612FNG 驱动直流风扇、板载 RGB LED
**人机界面**：0.96" I2C OLED（SSD1306）、USART1 串口调试输出

---

## 二、硬件架构

### 2.1 整体框图

```
                         STM32F407ZGTx (168MHz)
    ┌─────────────────────────────────────────────────┐
    │                                                   │
    │  DHT11 ──PA6─── GPIO (One-Wire)                   │
    │  MQ135 ──PA4─── ADC1_CH4 ┐                        │
    │         ──PC4─── GPIO IN ┤                        │
    │  MQ2   ──PA1─── ADC1_CH1 ├─ ADC1 DMA 双通道       │
    │         ──PC5─── GPIO IN ┘                        │
    │                                                   │
    │  OLED  ──PB6─── I2C1_SCL ┐                        │
    │         ──PB7─── I2C1_SDA ┤─ 400kHz 硬件 I2C      │
    │                                                   │
    │  TB6612──PB0─── TIM3_CH3 (PWM 10kHz)              │
    │         ──PC0─── AIN1 (方向)                       │
    │         ──PC1─── AIN2 (方向)                       │
    │         ──PC2─── STBY (使能)                       │
    │                                                   │
    │  LED   ──PF6─── R (低电平亮)                       │
    │         ──PF7─── G                                 │
    │         ──PF8─── B                                 │
    │                                                   │
    │  USART1──PA9─── TX ────→ USB-TTL → PC 串口助手     │
    │         ──PA10── RX                                │
    └─────────────────────────────────────────────────┘
```

### 2.2 时钟树

STM32F407 使用 **25MHz 外部晶振（HSE）**，经由内部 PLL 倍频后分发到各总线：

```
HSE (25MHz)
  └── PLL (M=25, N=336, P=2, Q=7)
        ├── SYSCLK  = 168MHz  = AHB1 → GPIO、DMA2
        ├── APB1    =  42MHz  → I2C1、TIM3
        │     └── TIM3_CLK = 2×APB1 = 84MHz（定时器时钟倍增）
        └── APB2    =  84MHz  → USART1、ADC1
              └── ADC_CLK  = APB2/4 = 21MHz（ADC 预分频）
```

**关键点**：APB1 上的定时器时钟在 APB1 预分频 ≠ 1 时会自动 ×2，所以 TIM3 实际获得 84MHz。ADC 时钟不可超过 36MHz，这里 21MHz 是合理值。

**浮点运算单元（FPU）**：Cortex-M4 内置单精度 FPU。`SystemInit()` 在启动阶段写入 CPACR 寄存器，使能 CP10/CP11 完全访问权限。此后 `float` 运算全部由硬件 FPU 执行，无需软件模拟，这是 MQ 传感器 PPM 计算中使用 `powf()` 能保持实时性的前提。

---

## 三、软件架构

### 3.1 分层结构

```
┌─────────────────────────────────────────┐
│  Application Layer (main.c)             │  ← 业务逻辑：采集→判断→控制→显示
├─────────────────────────────────────────┤
│  BSP Layer (bsp_*.c)                    │  ← 硬件抽象：封装外设初始化与操作
├─────────────────────────────────────────┤
│  StdPeriph Driver (Libraries/)          │  ← STM32 标准外设库 V1.8.0
├─────────────────────────────────────────┤
│  CMSIS (Libraries/CMSIS/)               │  ← Cortex-M 核心接口
├─────────────────────────────────────────┤
│  Hardware (STM32F407ZGTx)               │
└─────────────────────────────────────────┘
```

**工程采用 STM32 标准外设库（StdPeriph_Driver V1.8.0）** 而非 HAL 库。标准外设库直接操作寄存器，代码更紧凑，适合资源受限场景，但可移植性不如 HAL。

### 3.2 模块依赖关系

```
                 main.c
                   │
       ┌───────────┼───────────────┐
       │           │               │
    bsp_led    bsp_usart    bsp_dwt_delay
       │           │          │       │
       └───────────┘          │  core_delay (DWT CYCCNT)
                              │
              ┌───────────────┼───────────────┐
              │               │               │
          bsp_dht11       bsp_mq135       bsp_iic_debug
                              │               │
                          bsp_mq2         bsp_oled
                              │
                          bsp_tb6612
```

**初始化顺序约束**（由隐式依赖决定）：

| 顺序 | 模块 | 原因 |
|------|------|------|
| 1 | DWT_Delay | DHT11、OLED 初始化依赖微秒延时 |
| 2 | USART | printf 依赖 stdout 重定向到 USART1 |
| 3 | LED | 独立模块，无依赖 |
| 4 | DHT11 | GPIO 初始化，依赖 DWT 延时 |
| 5 | MQ135 | **配置 ADC1 + DMA2（供 MQ2 共用）** |
| 6 | MQ2 | 仅 GPIOC 输入，**依赖于 MQ135 已配置 ADC** |
| 7 | IIC | I2C1 初始化 |
| 8 | OLED | 依赖 I2C 总线就绪 |
| 9 | TB6612 | TIM3 独立，无依赖 |

---

## 四、核心模块详解

### 4.1 DWT 微秒延时（core_delay + bsp_dwt_delay）

**原理**：Cortex-M4 内核内置了 DWT（Data Watchpoint and Trace）单元，其中包含一个 CYCCNT（Cycle Count）寄存器，每个 CPU 时钟周期自动 +1。读取这个寄存器的当前值即可知道从复位起经过了多少个时钟周期。

**寄存器操作**（[core_delay.c](User/dwt_delay/core_delay.c)）：

```c
#define  DEM_CR      *(__IO uint32_t *)0xE000EDFC   // 调试异常与监控控制寄存器
#define  DWT_CYCCNT  *(__IO uint32_t *)0xE0001004   // DWT 周期计数寄存器
```

初始化分两步：
1. 写 `DEM_CR` 的 bit 24 (TRCENA) 置 1 —— 使能 DWT 模块的调试跟踪
2. 写 `DWT_CR` 的 bit 0 (CYCCNTENA) 置 1 —— 启动周期计数器

延时计算：
```
所需周期数 = 微秒数 × (CPU频率 / 1,000,000)
           = us × 168
```

`CPU_TS_Tmr_Delay_US()` 在 while 循环中持续读取 CYCCNT，累加增量直到达到目标周期数。该函数处理了计数器溢出回绕的情况（`tnow < told` 时加 `UINT32_MAX` 补偿）。

**意义**：在 168MHz 下，理论分辨率约 6ns。这使得 DHT11 单总线协议中数十微秒的精确时序成为可能。SysTick 也可以做延时，但最小分辨率受限于 SysTick 时钟（通常 1ms），无法满足 DHT11 的 30μs 级别时序。

---

### 4.2 DHT11 温湿度传感器（bsp_dht11）

**物理层**：DHT11 使用**单总线（One-Wire）协议**，仅需一根数据线（PA6），外接 4.7kΩ 上拉电阻到 3.3V。

**通信流程**（[bsp_dht11.c:61-109](User/dht11/bsp_dht11.c#L61-L109) ）：

```
主机                              DHT11
  │                                 │
  │── 拉低 20ms（起始信号）──────→  │  ① 主机通知 DHT11 准备读数
  │── 拉高 30μs ────────────────→  │  ② 释放总线
  │                                 │
  │←── 拉低 80μs（响应低电平）───   │  ③ DHT11 应答
  │←── 拉高 80μs（响应高电平）───   │  ④ DHT11 准备就绪
  │                                 │
  │←── 40 位数据（5 字节）────────   │  ⑤ 传输数据
  │                                 │
  │── 拉高（释放总线）──────────→  │  ⑥ 主机释放总线
```

**数据格式**（5 字节，共 40 位）：

| 字节 | 内容 | 示例 |
|------|------|------|
| Byte 0 | 湿度整数部分 | 45（表示 45%） |
| Byte 1 | 湿度小数部分 | 0（DHT11 小数固定为 0） |
| Byte 2 | 温度整数部分 | 25（表示 25°C） |
| Byte 3 | 温度小数部分 | 0（DHT11 小数固定为 0） |
| Byte 4 | 校验和 | = Byte0+Byte1+Byte2+Byte3 的低 8 位 |

**位读取时序**（`Read_Byte()`）：

每 bit 以 50μs 低电平开始，随后高电平持续时长决定是 "0" 还是 "1"：
- 高电平 ~26-28μs → bit = 0
- 高电平 ~70μs → bit = 1

代码在检测到低电平结束后延时 40μs 再读取引脚电平：如果此时已经是低电平，说明高电平短，是 "0"；如果此时仍是高电平，则等待它变低，标记为 "1"。

**GPIO 模式切换**：DHT11 通信的关键难点在于 PA6 引脚需要在输出模式（主机发起始信号）和输入模式（读取 DHT11 响应）之间动态切换。`DHT11_Mode_Out_PP()` 和 `DHT11_Mode_IPU()` 在运行时重新初始化 GPIO 模式。

---

### 4.3 MQ135 + MQ2 气体传感器（bsp_mq135 + bsp_mq2）

#### 4.3.1 ADC1 DMA 双通道架构

这是系统设计中最关键的架构决策。**MQ135 和 MQ2 共享 ADC1 及其 DMA 通道**，而非各自独立配置 ADC。

原因：
- STM32F407 有多达 3 个 ADC，但模拟输入引脚可能冲突
- 两个 MQ 传感器属于同一类设备（模拟气体传感器），采集周期相同
- 共用 ADC+DMA 减少中断开销，一次 DMA 传输即可更新两个通道的数据

**DMA 配置**（[bsp_mq135.c:44-64](User/mq135/bsp_mq135.c#L44-L64) ）：

| 参数 | 值 | 含义 |
|------|-----|------|
| 外设地址 | `ADC1 + 0x4C`（ADC_DR 寄存器） | 数据来源 |
| 内存地址 | `MQ_ADC_Values[2]` | 数据目标 |
| 传输方向 | 外设 → 内存 | |
| 缓冲大小 | 2（双通道） | |
| 模式 | 循环模式（Circular） | 持续更新 |
| 数据宽度 | 半字（16 bit） | 匹配 ADC 12-bit 分辨率 |

**ADC 扫描序列**：
```
[0] ADC1_CH1 (PA1) → MQ_ADC_Values[0] → MQ2 数据
[1] ADC1_CH4 (PA4) → MQ_ADC_Values[1] → MQ135 数据
```

DMA 在循环模式下不断将 ADC_DR 中的转换结果搬运到 `MQ_ADC_Values[]` 数组。应用程序在任何时刻读取数组都能获得最新的采样值，无需主动触发 ADC 转换。

**MQ135_Init() / MQ2_Init() 的分工**：
- `MQ135_Init()`：配置 GPIO（PA1/PA4 模拟输入 + PC4/PC5 数字输入）+ DMA2 + ADC1 全部
- `MQ2_Init()`：函数体为空 — 所有硬件初始化已在 MQ135_Init() 中完成

这就是为什么 main.c 中必须先调用 `MQ135_Init()` 再调用 `MQ2_Init()`。

#### 4.3.2 PPM 浓度计算

MQ 系列传感器是基于金属氧化物半导体（MOS）的电阻型传感器。测量原理：

**第一步：计算输出电压**
```
Vout = ADC_Value / 4096 × Vc
```
- `ADC_Value`：DMA 缓冲区中的 12-bit 采样值（0-4095）
- `Vc`：传感器供电电压（此处为 5.0V）
- 注意：MQ 传感器模块内部有 LDO，供电必须是 5V。PA4 是 5V 容忍引脚，但 ADC 参考电压是 3.3V

**第二步：计算传感器电阻 Rs**
```
Rs = (Vc - Vout) × RL / Vout
```
- `RL`：负载电阻（MQ135: 1kΩ, MQ2: 10kΩ）

**第三步：幂函数拟合 PPM**
```
PPM = A × (Rs / R0)^B
```
- `R0`：传感器在洁净空气中的标定电阻
- `A, B`：传感器特性曲线参数（由数据手册或实验标定得到）

| 参数 | MQ135 | MQ2 |
|------|-------|-----|
| RL | 1kΩ | 10kΩ |
| R0 | 2kΩ | 20kΩ |
| A | 4.17 | 43.03 |
| B | -2.28 | -1.66 |

**代码实现**（[bsp_mq135.c:107-111](User/mq135/bsp_mq135.c#L107-L111) ）：
```c
float vrl = (float)MQ_ADC_Values[1] / 4096.0f * MQ135_VC;
float Rs  = (MQ135_VC - vrl) * MQ135_RL / vrl;
return MQ135_A * powf(Rs / MQ135_R0, MQ135_B);
```

`powf()` 是硬件 FPU 加速的单精度幂函数，计算在微秒级完成，不阻塞主循环。

#### 4.3.3 DO 数字输出

MQ 传感器模块除了模拟输出（AO），还有数字阈值输出（DO）。模块上有电位器可调节阈值：
- **DO = LOW（0）**：气体浓度超过硬件阈值
- **DO = HIGH（1）**：正常

DO 引脚作为**冗余报警机制**，与软件 PPM 阈值并行判断。任何一个触发即启动风扇。

---

### 4.4 OLED SSD1306 显示屏（bsp_oled + bsp_iic）

#### 4.4.1 I2C 总线驱动（双模式设计）

[bsp_iic_debug.h](User/iic/bsp_iic_debug.h) 通过 `IIC_SELECT` 宏支持两种模式：

| IIC_SELECT | 模式 | 说明 |
|------------|------|------|
| 1 | 硬件 I2C | STM32 I2C1 外设，400kHz，DMA 未启用 |
| 0 | 软件 I2C | GPIO 模拟 I2C 时序，作为调试兼容方案 |

**硬件 I2C 配置**（`IIC_GPIO_Config()` 中 `#if (IIC_SELECT == 1)` 分支）：
- PB6/PB7 配置为复用功能开漏输出（AF_OD），映射到 I2C1
- 时钟 400kHz（Fast Mode），占空比 2:1
- 从机地址设为 0x78（SSD1306 默认 7-bit 地址）
- 外接 4.7kΩ 上拉电阻（开漏输出必须）

**软件 I2C** 通过 GPIO 直接操作 PB6/PB7，手动模拟起始条件、停止条件、字节发送/接收。OLED 驱动代码实际上**绕过软件 I2C API**，直接在 `IIC_Write_Byte()` 中使用 STM32 标准外设库的 I2C 函数，这意味着当前 OLED 通信仅在 `IIC_SELECT=1`（硬件模式）下工作。

#### 4.4.2 SSD1306 初始化序列

SSD1306 是 Solomon Systech 的 128×64 单色 OLED 控制器。初始化需要发送约 20 条命令（[bsp_oled.c:41-63](User/oled/bsp_oled.c#L41-L63) ）：

| 命令 | 作用 |
|------|------|
| 0xAE/0xAF | 显示关闭/开启 |
| 0x81 + 0xFF | 对比度设到最大 |
| 0x20 + 0x10 | 页寻址模式（每页 8 行，共 8 页） |
| 0xA1 | 段重映射（左右镜像） |
| 0xC8 | COM 扫描方向（上下翻转） |
| 0xA8 + 0x3F | 多路复用比 64（64 行） |
| 0x8D + 0x14 | 使能内部电荷泵（升压至 ~7.5V 驱动 OLED） |

**显示原理**：128×64 像素被分为 8 页（Page 0-7），每页 8 行。每一列中，一个字节的 8 位对应 8 个像素。写入过程：
1. `OLED_SetPos(x, y)` 设置当前页（y）和列（x）
2. 后续的 `Oled_Write_Data()` 自动递增列地址

#### 4.4.3 字库系统

[bsp_oled_codetab.h](User/oled/bsp_oled_codetab.h) 提供三级字体：

| 字体 | 尺寸 | 字符数 | 用途 |
|------|------|--------|------|
| F6x8 | 6×8 像素 | 95 个 ASCII | 紧凑信息显示 |
| F8X16 | 8×16 像素 | 95 个 ASCII | 标题显示 |
| F16x16 | 16×16 像素 | 4 个中文 | "野火科技" |

每个字符的数据是位图（bitmap），例如 F8X16 的 'A' 字符占用 16 字节。`OLED_ShowStr()` 遍历字符串，查表取字模，写入 OLED。

#### 4.4.4 页缓冲区残留问题与修复

SSD1306 在页寻址模式下，写操作**仅覆盖指定起始列 + 数据字节数的区域**，未写入的列保持上一次的像素值。这导致一个典型问题：当可变内容缩短时（文字变短、数字位数减少、状态切换），旧内容的像素不会被自动清除，形成"鬼影"（ghosting）。

**问题场景分析**：

| 场景 | 原因 | 残留位置 |
|------|------|---------|
| 报警解除后 | `OLED_ShowAlarm()` 用 8×16 大字写 Page 6+7。正常状态 `OLED_ShowGas()` 用 6×8 小字只写 Page 6，**Page 7 完全不碰** | Page 7（第 56-63 行）残留 "** ALARM **" 下半截 |
| DHT11 故障恢复 | 故障文本 "DHT11: --" 覆盖列 0-53，正常温湿度数据显示在离散的固定列位置，中间有间隙 | Page 2 列 12-15, 30-39, 54-63 |
| FAN:OFF → FAN:ON | "FAN:OFF" = 7 字符 (42px)，"FAN:ON" = 6 字符 (36px) | Page 2 列 116-121 残留 'F' |
| 温度从 25→5 | 两位数变一位数，"25" 的 '5' 在列 22-27 不被覆盖 | Page 2 显示 "55" |

**修复方案**（V1.1→V1.2）：

新增两个 OLED 驱动函数（[bsp_oled.c](User/oled/bsp_oled.c)）：

| 函数 | 作用 |
|------|------|
| `OLED_ClearPage(y)` | 清空一整页（128 列 × 8 行），用于报警→正常时清除 Page 7 |
| `OLED_FillBytes(x, y, len, 0x00)` | 从 (x, y) 开始写入 `len` 个零字节，用于清除局部队列间隙 |

`OLED_ShowEnv()` 重写为**先清除再写入**模式：每个数值字段前用 `OLED_FillBytes` 清空其最大可能宽度（整数位 3 字符 = 18 像素，小数位 1 字符 = 6 像素），每个字段间间隙也用 `OLED_FillBytes` 清零。这同时解决了整数位数变化和 DHT11 故障恢复两类残留。

风扇状态显示改为固定宽度：`"FAN:ON "` 末尾补空格，使 ON/OFF 均为 7 字符。

报警解除时通过追踪 `prev_alarm_active` 状态位，仅在报警→正常转换瞬间调用 `OLED_ClearPage(7)`，避免每周期无意义清页。

---

### 4.5 TB6612FNG 电机驱动（bsp_tb6612）

TB6612FNG 是东芝的双通道 H 桥电机驱动 IC，本次仅使用通道 A 驱动单个直流风扇。

**控制接口**：

| 信号 | STM32 引脚 | 功能 |
|------|-----------|------|
| PWMA | PB0 (TIM3_CH3) | PWM 调速，10kHz |
| AIN1 | PC0 | 方向控制 |
| AIN2 | PC1 | 方向控制 |
| STBY | PC2 | 使能（高电平=工作） |

**H 桥真值表**（AIN1/AIN2 组合）：

| AIN1 | AIN2 | 电机状态 |
|------|------|---------|
| 0 | 0 | 短路制动（Stop） |
| 0 | 1 | 正转（Forward） |
| 1 | 0 | 反转（Backward） |
| 1 | 1 | 禁止 |

**PWM 频率计算**（[bsp_tb6612.c:39-43](User/tb6612/tb6612.c#L39-L43) ）：

```
PWM 频率 = TIM3_CLK / (PSC × ARR)
         = 84,000,000 / (84 × 100)
         = 10,000 Hz = 10kHz
```

10kHz 是电机 PWM 的常用频率：高于人耳听觉范围（避免啸叫），低于 MOSFET 开关损耗显著增加的频率（~20kHz+）。

**占空比控制**：`TB6612_SetSpeed(duty)` 直接写 `TIM3->CCR3 = duty`。
- ARR = 100，所以 duty ∈ [0, 100] 恰好对应 0%-100%
- 风扇启动时使用 60% 占空比（`FAN_SPEED` 宏）

---

### 4.6 USART 串口重定向（bsp_usart）

USART1 配置为 115200-8-N-1，PA9（TX）/ PA10（RX）。

**printf 重定向** 是关键技巧。通过重写 C 标准库的 `fputc()`：

```c
int fputc(int ch, FILE *f) {
    USART_SendData(DEBUG_USART, (uint8_t)ch);
    while (USART_GetFlagStatus(DEBUG_USART, USART_FLAG_TXE) == RESET);
    return ch;
}
```

此后项目中任何 `printf()` 调用都会将字符逐字节发送到 USART1 数据寄存器。配合 Keil 的 MicroLIB（精简 C 库），无需完整的 `semihosting` 支持即可使用标准 I/O 函数。

---

### 4.7 RGB LED（bsp_led）

霸天虎开发板板载三颗 LED：PF6（红）、PF7（绿）、PF8（蓝），低电平点亮。

[bsp_led.h](User/led/bsp_led.h) 定义了便捷的颜色宏：
```c
#define LED_RED     { LED1_ON; LED2_OFF; LED3_OFF; }
#define LED_GREEN   { LED1_OFF; LED2_ON; LED3_OFF; }
```

**注**：代码中 LED 宏使用 `{ }` 包裹是必要的 —— 这是从 V1.0 迭代中修复的 bug。无括号的宏在 `if/else` 中展开时会导致语法错误。

---

## 五、主程序流程（main.c）

### 5.1 系统初始化

```
main()
  │
  ├─ DWT_Delay_Init()        使能 CYCCNT，微秒延时就绪
  ├─ USART_Config()          PA9/PA10, 115200, printf 可用
  ├─ LED_GPIO_Config()       PF6/7/8 推挽输出
  ├─ DHT11_GPIO_Config()     PA6 推挽输出（初始高）
  ├─ MQ135_Init()            PA4 模拟输入 + ADC1 DMA 双通道配置
  ├─ MQ2_Init()              （空函数，ADC 已在上面配好）
  ├─ IIC_GPIO_Config()       PB6/PB7, I2C1 400kHz 开漏
  ├─ OLED_Init()             SSD1306 完整初始化序列
  ├─ TB6612_Init()           PB0 TIM3_CH3 PWM + PC0/1/2 GPIO
  │
  └─ while(1) 主循环
```

每步初始化完成后通过 `printf` 输出状态确认。

### 5.2 主循环（2 秒周期）

```c
while (1) {
    // ① 读取 DHT11
    uint8_t dht11_ok = Read_DHT11(&dht11);
    if (dht11_ok) {
        temp = dht11.temp_int + dht11.temp_deci * 0.1f;
        humi = dht11.humi_int + dht11.humi_deci * 0.1f;
    } else {
        temp = 0; humi = 0;   // 读取失败时归零
    }

    // ② 读取 MQ 传感器
    mq135_ppm = MQ135_Get_PPM();    // MQ_ADC_Values[1] → PPM 转换
    mq2_ppm   = MQ2_Get_PPM();      // MQ_ADC_Values[0] → PPM 转换
    mq135_do  = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4);
    mq2_do    = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_5);

    // ③ 综合报警判断
    alarm_active = CheckAlarm(temp, mq135_ppm, mq2_ppm, mq135_do, mq2_do);

    // ④ 风扇控制
    if (alarm_active && !fan_running) {
        TB6612_Forward();
        TB6612_SetSpeed(60);
        fan_running = 1;
    } else if (!alarm_active && fan_running) {
        TB6612_Stop();
        fan_running = 0;
    }

    // ⑤ LED 指示
    if (alarm_active) { LED_RED; } else { LED_GREEN; }

    // ⑥ 串口输出
    printf("T=%.1fC H=%.1f%% MQ135=%.1fppm MQ2=%.1fppm ...\r\n", ...);

    // ⑦ OLED 刷新
    OLED_ShowTitle();
    if (dht11_ok) OLED_ShowEnv(...); else OLED_ShowStr(0,2,"DHT11: --",1);
    if (alarm_active) OLED_ShowAlarm(); else OLED_ShowGas(...);
    OLED_ShowFan(fan_running);

    DWT_Delay_ms(2000);  // 2 秒采集间隔
}
```

### 5.3 报警判断逻辑（CheckAlarm）

报警采用**五路并行判断**，满足任一条件即触发：

| 检测项 | 判断条件 | 阈值宏 |
|--------|---------|--------|
| 温度过高 | `temp > 30°C` | `TEMP_ALARM` |
| MQ135 模拟超标 | `mq135_ppm > 500` | `MQ135_ALARM` |
| MQ2 模拟超标 | `mq2_ppm > 5000` | `MQ2_ALARM` |
| MQ135 数字触发 | `mq135_do == 0` | 硬件电位器 |
| MQ2 数字触发 | `mq2_do == 0` | 硬件电位器 |

**设计理由**：软件 PPM 阈值和硬件 DO 阈值双重保障。即使 ADC 或 PPM 标定不准确，模块上的数字阈值仍能提供硬件级别的保护。

---

## 六、关键技术要点

### 6.1 为什么用 DWT 而非 SysTick 做延时？

SysTick 是 Cortex-M 内核的标准定时器，通常用于 RTOS 心跳（1ms tick）。用它做微秒级延时的核心问题：
- **最小分辨率**：SysTick 通常配 1ms，2MHz 下最小也是 0.5μs，但每次重载有开销
- **上下文切换**：如果有 RTOS，SysTick 可能被用于任务调度，干扰延时精度
- **DWT CYCCNT 的优势**：纯硬件计数器，6ns 分辨率，零软件开销，不与任何外设冲突

DHT11 单总线协议中 30μs、40μs 级别的时序是 SysTick 难以保证精确的。DWT + 忙等待是嵌入式场景下最可靠的微秒延时方案。

### 6.2 为什么 ADC1 不采集 5V MQ 信号而是分压？

MQ 传感器模块输出 0-5V 模拟信号（与供电电压相关），但 STM32F407 的 ADC 参考电压（VREF+）通常接 3.3V（VDDA）。PA4/PA1 引脚具有 5V 容忍特性（FT），但 ADC 输入电压不得超过 VREF+，否则会损坏 ADC 或读数饱和。

**实际方案**（README 建议）：在 MQ135/MQ2 的 AO 引脚上接入 **10kΩ + 20kΩ 电阻分压网络** → GND，使最大输出电压从 5V 降至 5V × 20k/(10k+20k) ≈ 3.3V。但这会损失精度，需要在标定时将此分压系数纳入计算。

当前代码未在 PPM 公式中显式处理分压系数，学员可根据实际是否添加了分压电阻决定是否需要乘以分压比的倒数（1.5×）。

### 6.3 为什么传感器需要"预热"？

MQ 系列传感器的敏感材料（SnO₂ 金属氧化物半导体）需要加热到 300-400°C 才能正常工作。模块内部有加热丝，通电后需要 **1-2 分钟** 才能达到稳定工作温度。在此期间读数会从 0 逐渐上升并趋于稳定，不应触发报警判断。

当前代码未实现上电预热等待逻辑（直接进入 2 秒循环），实际使用中前 1-2 分钟的报警可能是误报。

### 6.4 MicroLIB 的必要性

Keil MDK 提供两种 C 运行时库：
- **Standard C Library**：完整 C99 标准库，占用 Flash 较大
- **MicroLIB**：精简 C 库，专为嵌入式优化，代码体积减少约 50%

本项目使用 MicroLIB 的主要原因：
1. `printf` / `powf` 所需的标准库函数在 MicroLIB 中均有精简实现
2. 不使用 MicroLIB 会导致链接错误（`__use_no_semihosting` 未定义等问题）
3. STM32F407 有 1MB Flash，但精简仍是好习惯

---

## 七、代码统计

| 模块 | 源文件 | 代码行数 | 功能 |
|------|--------|---------|------|
| main | main.c | ~240 | 主循环 + 显示 + 报警 |
| DHT11 | bsp_dht11.c/h | ~110 | 单总线温湿度读取 |
| MQ135 | bsp_mq135.c/h | ~110 | ADC DMA 双通道 + PPM |
| MQ2 | bsp_mq2.c/h | ~20 | PPM 计算（ADC 复用） |
| OLED | bsp_oled.c/h + codetab | ~400+ | SSD1306 显示 + 字库 |
| I2C | bsp_iic_debug.c/h | ~200 | 硬件/软件双模 I2C |
| TB6612 | bsp_tb6612.c/h | ~85 | PWM 电机控制 |
| USART | bsp_usart.c/h | ~45 | 串口 printf 重定向 |
| LED | bsp_led.c/h | ~25 | RGB LED 控制 |
| DWT | core_delay.c/h + bsp_dwt_delay | ~60 | 微秒延时 |
| **合计** | 14 个 .c / 14 个 .h | ~1300 | |

其中 `Libraries/` 下的 STM32F4xx_StdPeriph_Driver 为标准外设库，不计入自写代码。

---

## 八、开发环境

| 组件 | 版本 | 说明 |
|------|------|------|
| IDE | Keil MDK-ARM 5.38 | 工程文件 `.uvprojx` |
| 编译器 | ARMCC V5.06 update 7 | **不可升级到 V6（AC6）** |
| 固件库 | STM32F4xx_StdPeriph_Driver V1.8.0 | 标准外设库，非 HAL |
| 调试器 | Fire CMSIS-DAP (SWD) | 野火 DAP 仿真器 |
| 芯片 | STM32F407ZGTx | Cortex-M4, 1024KB Flash, 192KB SRAM |

---

## 九、版本演进

| 版本 | 日期 | 变更 |
|------|------|------|
| V1.0 | — | 首次发布，6 种演示模式分别验证各模块 |
| V1.1 | 2026-06 | 重构为统一环境监测系统，集成全部传感器 + 自动报警 |
| V1.2 | 2026-06 | 修复 OLED 页缓冲区残留问题（4 处），新增 `OLED_ClearPage` / `OLED_FillBytes` |

V1.0 → V1.1 的主要改进：
1. 将分散的演示模式合并为单一实时监测循环
2. 统一引脚分配（文档与代码一致）
3. 修复 LED 宏在 if/else 中的编译错误（加 `{ }` 包裹）
4. 修复 F16x16 中文字库 `const` 限定符不一致导致的编译警告
5. 添加 `MQ_ADC_Values[]` 共享 DMA 缓冲区架构

V1.1 → V1.2 的主要改进：
1. 新增 `OLED_ClearPage()` / `OLED_FillBytes()` 两个页面清除函数
2. `OLED_ShowEnv()` 改为先清后写，彻底消除整数字宽和 DHT11 错误恢复的残留
3. 风扇状态 "FAN:ON" 末尾补空格，固定 7 字符宽度
4. 报警→正常转换时自动清除 Page 7（8×16 大字下半截残留）
