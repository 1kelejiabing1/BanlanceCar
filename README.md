# BalanceCar 两轮自平衡小车

<div align="center">

基于 **STM32F103C8T6** + **FreeRTOS** 的两轮自平衡小车，支持三环 PID 控制、蓝牙实时调参、NRF24L01 无线遥控与数据回传、OLED 状态显示。

`MPU6500 姿态解算 · 互补滤波 · 直立环/速度环/转向环 PID · FreeRTOS 多任务`

</div>

---

## 目录

- [功能特性](#功能特性)
- [硬件清单](#硬件清单)
- [引脚分配](#引脚分配)
- [软件架构](#软件架构)
- [目录结构](#目录结构)
- [快速开始](#快速开始)
- [使用说明](#使用说明)
- [蓝牙调参协议](#蓝牙调参协议)
- [NRF24L01 遥控协议](#nrf24l01-遥控协议)
- [PID 参数说明](#pid-参数说明)
- [踩坑记录](#踩坑记录)
- [许可证](#许可证)

---

## 功能特性

- **直立平衡**：三环串级 PID（直立环 → 速度环 → 转向环），控制周期 15ms / 60ms
- **姿态解算**：MPU6500 六轴传感器 + 一阶互补滤波，融合加速度计与陀螺仪得到俯仰角
- **蓝牙调参**：通过手机 App 发送 `slider` / `joystick` 指令，实时修改 PID 参数、遥控小车
- **无线遥控**：NRF24L01 收发数据包，支持速度 / 姿态数据回传
- **OLED 显示**：SSD1306 128x64，实时显示目标速度、实际速度、角度等信息
- **按键控制**：4 个按键，支持单击 / 双击 / 长按，可启动、停止、手动调速
- **FreeRTOS 多任务**：分层清晰，任务间通过队列 / 信号量通信

---

## 硬件清单

| 模块 | 型号 / 说明 | 数量 |
| --- | --- | --- |
| 主控 | STM32F103C8T6（LQFP48，Cortex-M3，72MHz） | 1 |
| 陀螺仪加速度计 | MPU6500（I2C 接口） | 1 |
| 无线模块 | NRF24L01+（SPI 接口） | 1 |
| 蓝牙模块 | HC-05 / BLE 透传模块（UART，9600） | 1 |
| 显示屏 | OLED SSD1306 128x64（软件 I2C） | 1 |
| 电机驱动 | TB6612FNG（PWM + 方向控制） | 1 |
| 电机 | 带霍尔编码器的减速直流电机（减速比约 9.28） | 2 |
| 按键 | 轻触按键 | 4 |
| LED | 板载 / 外接指示灯 | 1 |
| 电源 | 7.4V / 11.1V 锂电池（降压给主控供电） | 1 |

---

## 引脚分配

> 引脚定义在 `Core/Inc/main.h` 中，由 STM32CubeMX 生成。

| 外设 | 引脚 | 说明 |
| --- | --- | --- |
| LED | PC13 | 状态指示灯 |
| 左电机 PWM | PA0（TIM2_CH1） | 左轮 PWM |
| 右电机 PWM | PA1（TIM2_CH2） | 右轮 PWM |
| 左电机方向 | PB12（AIN1）/ PB13（AIN2） | TB6612 方向 |
| 右电机方向 | PB14（BIN1）/ PB15（BIN2） | TB6612 方向 |
| 左编码器 | PA6（TIM3_CH1）/ PA7（TIM3_CH2） | 霍尔编码器 |
| 右编码器 | PB6（TIM4_CH1）/ PB7（TIM4_CH2） | 霍尔编码器 |
| MPU6500 | PB10（I2C2_SCL）/ PB11（I2C2_SDA） | 姿态传感器 |
| NRF24L01 | PB3（SPI1_SCK）/ PB4（MISO）/ PB5（MOSI）<br>PA8（CE）/ PA15（CSN） | 无线收发 |
| 蓝牙 | PA2（USART2_TX）/ PA3（USART2_RX） | HC-05 透传 |
| OLED | PB8（SCL）/ PB9（SDA） | 软件模拟 I2C |
| 按键 K1 | PB1 | 启动 / 停止平衡 |
| 按键 K2 | PB0 | 停止电机 |
| 按键 K3 | PA5 | 手动加速 |
| 按键 K4 | PA4 | 手动减速 |
| 调试串口 | PA9（USART1_TX）/ PA10（USART1_RX） | 预留调试 |

---

## 软件架构

采用经典的四层分层架构，自上而下为：

```
┌─────────────────────────────────────────────┐
│  App 层（应用任务，FreeRTOS task）            │
│  task_led / task_key / task_control /        │
│  task_ble / task_nrf24l01 / task_oled ...    │
├─────────────────────────────────────────────┤
│  Middleware 层（协议 / 算法）                 │
│  middleware_pid / middleware_ble /           │
│  middleware_nrf24l01                         │
├─────────────────────────────────────────────┤
│  Driver 层（外设驱动）                        │
│  driver_mpu6500 / driver_motor /             │
│  driver_encoder / driver_nrf24l01 /          │
│  driver_ble / driver_oled                    │
├─────────────────────────────────────────────┤
│  Bsp 层（板级支持）                           │
│  bsp_key / bsp_led / bsp_uart                │
├─────────────────────────────────────────────┤
│  HAL 层（STM32CubeMX 生成）                   │
└─────────────────────────────────────────────┘
```

### 控制框图

```
            ┌──────────┐ 速度环输出(目标角度)  ┌──────────┐
 目标速度 ──▶ 速度环PID │ ───────────────────▶ 直立环PID │──┐
            └──────────┘                      └──────────┘  │
                                                     ▲       │ 角度
 转向环PID ──▶ 差速 ──▶ 左/右轮目标速度                    │       ▼
              ┌──────────────────────────────────────────────┐
 目标转向角 ─▶│ 左轮目标速度 = 平均速度 + 差速/2                │
              │ 右轮目标速度 = 平均速度 - 差速/2                │
              └──────────────────────────────────────────────┘
```

- **直立环（角度环）**：15ms 周期，PID 控制维持车体直立
- **速度环**：60ms 周期，PID 输出作为直立环的目标角度，实现前进 / 后退
- **转向环**：60ms 周期，PID 输出作为左右轮差速，实现转向
- **姿态解算**：一阶互补滤波 `angle = α·angle_acc + (1-α)·angle_gyro`

### FreeRTOS 任务一览

| 任务 | 入口函数 | 优先级 | 栈大小(words) | 周期 | 功能 |
| --- | --- | --- | --- | --- | --- |
| task_led | `task_led` | 1 | 8 | 500ms | LED 心跳 |
| task_key | `task_key` | 2 | 128 | 10ms | 按键状态机 |
| task_control | `task_control` | 5 | 256 | 15ms | 三环 PID 控制 |
| task_middleware_ble | `task_middleware_ble` | 2 | 128 | 事件驱动 | BLE 字节解析成帧 |
| task_ble | `task_ble` | 3 | 256 | 事件驱动 | BLE 指令处理 |
| task_nrf24l01 | `task_nrf24l01` | 4 | 256 | 10ms | 无线收发 |
| task_oled | `task_oled` | 1 | 128 | 60ms | OLED 刷新 |


---

## 目录结构

```
BalanceCar/
├── BalanceCar.ioc                 # STM32CubeMX 工程配置
├── Core/                          # CubeMX 生成：main、外设初始化、中断
│   ├── Inc/                       #   main.h、外设头文件
│   └── Src/                       #   main.c、freertos.c、各外设 .c
├── Drivers/                       # ST HAL 库 + CMSIS
├── Middlewares/                   # FreeRTOS
├── User/                          # ★ 用户代码（本项目的核心）
│   ├── App/                       # 应用层任务
│   │   ├── app_task.c             #   任务创建入口
│   │   ├── app_debug.c            #   调试输出（printf 重定向）
│   │   ├── task_led.c             #   LED 任务
│   │   ├── task_key.c             #   按键任务
│   │   ├── task_control.c         #   控制任务（三环 PID + 互补滤波）
│   │   ├── task_ble.c             #   蓝牙指令处理任务
│   │   ├── task_middleware_ble.c  #   蓝牙字节解析任务
│   │   ├── task_nrf24l01.c        #   无线收发任务
│   │   └── task_oled.c            #   OLED 刷新任务
│   ├── Bsp/                       # 板级支持
│   │   ├── bsp_key.c              #   按键驱动（状态机 + 消抖）
│   │   ├── bsp_led.h              #   LED 宏
│   │   └── bsp_uart.c             #   串口接收（预留）
│   ├── Driver/                    # 外设驱动
│   │   ├── driver_mpu6500.c       #   MPU6500（I2C）
│   │   ├── driver_motor.c         #   电机（PWM + 方向）
│   │   ├── driver_encoder.c       #   编码器（定时器计数）
│   │   ├── driver_nrf24l01.c      #   NRF24L01（SPI）
│   │   ├── driver_ble.c           #   蓝牙（UART）
│   │   └── driver_oled.c          #   OLED（软件 I2C）
│   └── Middleware/                # 协议 / 算法中间件
│       ├── middleware_pid.c       #   位置式 PID
│       ├── middleware_ble.c       #   蓝牙协议解析（环形缓冲 + 状态机）
│       └── middleware_nrf24l01.c  #   无线数据打包 / 解包
├── MDK-ARM/                       # Keil MDK 工程文件
└── 遇到的问题.txt                  # 开发过程中的踩坑记录
```

---

## 快速开始

### 环境要求

- **Keil MDK-ARM**（推荐 5.36 及以上），或 STM32CubeIDE + GCC
- **STM32CubeMX**（用于查看 / 修改 `.ioc` 配置）
- 调试器：ST-Link / J-Link / DAP-Link
- 工具链：`MDK-ARM/BalanceCar.uvprojx` 已配置好 Keil 工程

### 编译与烧录

1. 克隆仓库
   ```bash
   git clone https://github.com/<your-name>/BalanceCar.git
   ```
2. 用 Keil 打开 `MDK-ARM/BalanceCar.uvprojx`
3. 选择正确的目标芯片（STM32F103C8），确认 Flash Download 配置（128KB）
4. 点击 **Build（F7）** 编译，**Download（F8）** 烧录
5. 上电后，OLED 显示启动界面，LED 以 500ms 周期闪烁

### 调试输出

- 调试信息通过 `printf` 重定向输出（见 `User/App/app_debug.h`）：
  - `DEBUG_ENABLED = 1`：输出到串口（`app_debug.c` 中的 `DEBUG_UART`）
  - `DEBUG_ENABLED = 2`：输出到蓝牙（默认，方便手机端查看）
  - `DEBUG_ENABLED = 3`：关闭输出

---

## 使用说明

### 按键

| 按键 | 单击 | 说明 |
| --- | --- | --- |
| K1 | 启动 / 停止平衡 | 切换 `run_flag`，启动或停止直立控制 |
| K2 | 停止电机 | 直接关闭电机 |
| K3 | 加速 | 手动增加目标速度（每次 +10） |
| K4 | 减速 | 手动减少目标速度（每次 -10） |

> 按键支持单击、双击、长按、长按保持四种事件，详见 `bsp_key.c` 状态机。

### 启动步骤

1. 上电，等待 MPU6500 初始化完成
2. 将小车竖直放置（尽量接近平衡角度）
3. 按下 **K1** 启动平衡控制
4. 通过蓝牙 / NRF24L01 遥控移动小车
5. 车体倾斜超过 ±50° 时自动停止电机（安全保护）

---

## 蓝牙调参协议

蓝牙透传数据以 `[` 开头、`]` 结尾，字段用 `,` 分隔，例如 `[slider,AngleKp,4.5]`。

### 1. 滑杆调参（`slider`）

格式：`[slider,<参数名>,<数值>]`

| 参数名 | 说明 |
| --- | --- |
| `AngleKp` / `AngleKi` / `AngleKd` | 直立环（角度环）PID |
| `AngleOffset` | 直立环输出偏移 |
| `SpeedKp` / `SpeedKi` / `SpeedKd` | 速度环 PID |
| `SpeedOffset` | 速度环输出偏移 |
| `TurnKp` / `TurnKi` / `TurnKd` | 转向环 PID |
| `TurnOffset` | 转向环输出偏移 |

### 2. 摇杆遥控（`joystick`）

格式：`[joystick,<左横>,<左纵>,<右横>,<右纵>]`

- 左摇杆纵向 → 目标速度（速度环 Target）
- 右摇杆横向 → 目标转向（转向环 Target）

---

## NRF24L01 遥控协议

### 接收（遥控端 → 小车）

| 字节 | 含义 |
| --- | --- |
| `[0]` | 模式：0=仅发送，1=发送 + 回传 |
| `[1]` | 左摇杆横向 |
| `[2]` | 左摇杆纵向 |
| `[3]` | 右摇杆横向 |
| `[4]` | 右摇杆纵向 |

### 发送（小车 → 遥控端）

| 字节 | 含义 |
| --- | --- |
| `[0]` | 数据包类型（CAR_DATA） |
| `[1]` | 左轮目标速度 |
| `[2]` | 右轮目标速度 |
| `[4:7]` | 角度（float） |
| `[8:11]` | 左轮实际速度（float） |
| `[12:15]` | 右轮实际速度（float） |

> 收发双方地址、射频通道、数据包宽度等参数必须完全一致，配置见 `driver_nrf24l01_init()`。

---

## PID 参数说明

默认参数定义在 `User/App/task_control.c` 中，可通过蓝牙实时修改：

| 环 | Kp | Ki | Kd | 输出限幅 | 说明 |
| --- | --- | --- | --- | --- | --- |
| 直立环（角度环） | 4.0 | 0.15 | 5.0 | ±100 | 维持车体直立 |
| 速度环 | 1.1 | 0.05 | 0 | ±20 | 控制前进 / 后退速度 |
| 转向环 | 4.0 | 3.0 | 0 | ±50 | 控制转向 |

> 调参建议：先固定速度环和转向环为 0，只调直立环使车体能稳定直立；再加入速度环，最后调转向环。

---

## 踩坑记录

开发过程中遇到的问题与解决方案整理在 [`遇到的问题.txt`](./遇到的问题.txt)，摘要如下：

1. **串口发送数据不完整** —— 多任务同时打印导致数据混乱，需保证串口独占或加锁。
2. **NRF24L01 无法收发** —— SPI `Size` 参数未计入 1 字节读写指令，需在原值上加 1。
3. **发送标志位清除位置错误** —— 标志位清除放进了发送 `while` 循环，导致发送失败。
4. **加入蓝牙任务后 HardFault** —— 队列在低优先级任务中创建，高优先级任务先用了未创建的 NULL 队列；统一把 `*_init` 放在任务创建之前，`*_start` 放在真正需要时。
5. **按键任务无法工作** —— 控制任务耗时过长（11~12ms）；最终定位到 OLED 软件 I2C 刷新过慢，将 OLED 刷新拆分为独立任务解决。
6. **蓝牙曲线数据异常** —— 波特率过高导致手机 App 接收不过来，改回 9600 解决。
7. **电机嗡嗡响、转动异常** —— 电机速度参数误用 `float` 传入 `int` 形参，改回 `int` 解决。
8. **小车无法保持平衡** —— 电池没电了，充电即可。

---

## 许可证

本项目暂未指定开源许可证。若需开源，建议选择 [MIT License](https://opensource.org/licenses/MIT)（宽松，允许自由使用和修改）。

> 注意：`Drivers/` 下的 ST HAL 库、`Middlewares/` 下的 FreeRTOS 属于第三方代码，请遵循各自的许可证（BSD / MIT 风格）。

---

## 致谢

- [STMicroelectronics / STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html)
- [FreeRTOS](https://www.freertos.org/)
- 感谢所有参考过的开源平衡小车项目与教程
