# STM32F103C8 环境监测系统 - AI 编码助手指南

## 项目概述
基于 STM32F103C8 的嵌入式环境监测系统，使用 ST7735 LCD 显示屏、DS18B20 温度传感器、LDR 光照传感器、W25Q64 Flash 存储器和蜂鸣器。实现了多级菜单界面、数据记录、实时监测和音乐播放功能。

## 架构设计

### 分层结构
- **CMSIS 层** (`CMSIS/`): ARM Cortex-M3 核心和 STM32F10x 设备支持
- **标准外设库** (`Libraries/`): ST 官方 SPL (Standard Peripheral Library)
- **系统层** (`System/`): 基础系统服务（定时器、延时、ADC、RTC）
- **硬件层** (`Hardware/`): 外设驱动（LCD、传感器、SPI、存储）
- **应用层** (`User/`): 主程序和中断服务

### 关键数据流
1. **传感器采样**: TIM2 (1秒周期) → 触发传感器读取 → 更新全局变量 `Temp`/`Light`
2. **数据存储**: TIM4 (10ms周期) → 累计计时器 `TimerCount` → 达到 `SaveInterval` 时触发 W25Q64 写入
3. **菜单系统**: 按键轮询 → 状态机切换 → LCD 刷新显示
4. **中断处理**: `Menu.c` 中的 `TIM2_IRQHandler`/`TIM4_IRQHandler` 覆盖 `Timer.c` 中的弱定义

## 开发工作流

### 构建与烧录 (使用 Embedded IDE 扩展)
- **构建**: 运行任务 `build` 或执行命令 `${command:eide.project.build}`
- **烧录**: 运行任务 `flash` (使用 ST-LINK) 或 `build and flash` (构建后烧录)
- **清理**: 运行任务 `clean` 清除生成文件
- **输出**: 编译产物在 `Project/build/Target 1/` (包含 `.axf`/`.hex`/`.map` 文件)

### 调试配置
- 调试配置文件在 `Project/DebugConfig/`，使用 ST-LINK 烧录器
- 设备型号: STM32F103C8 (64KB Flash, 20KB SRAM)
- 时钟: 72MHz 主频 (通过 `system_stm32f10x.c` 配置 PLL)

### 项目文件结构
- **项目配置**: `Project/.eide/eide.yml` (Embedded IDE 项目文件)
- **Keil 工程**: `Project/Project.uvprojx` (保留用于 Keil MDK 兼容)
- **源文件路径**: 所有源码在上级目录 (`../CMSIS`, `../Hardware` 等)，通过相对路径引用

## 编码约定

### 外设初始化模式
遵循标准初始化顺序（见 `main.c`）：
```c
NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  // 全局只配置一次
LCD_Init();      // 显示优先初始化（用于调试信息）
传感器初始化...
数据存储初始化...
Timer_Init();    // 定时器最后初始化（避免未就绪中断）
```

### GPIO 配置规范
使用宏定义管理引脚（见 `TempSensor.h`）：
```c
#define DS18B20_GPIO_PORT    GPIOA
#define DS18B20_GPIO_PIN     GPIO_Pin_1
#define DS18B20_GPIO_CLK     RCC_APB2Periph_GPIOA
#define DS18B20_High         GPIO_SetBits(DS18B20_GPIO_PORT, DS18B20_GPIO_PIN)
```

### 中断优先级分组
使用分组 2 (2位抢占/2位响应)：
- TIM2: 抢占优先级 2, 响应优先级 1 (数据采样)
- TIM4: 抢占优先级 2, 响应优先级 2 (计时器)

### 全局变量通信
关键全局变量使用 `extern` 跨文件共享（`main.c` 定义，`Menu.c` 使用）：
```c
extern float Temp;              // 温度数据
extern uint16_t Light;          // 光照数据
extern uint8_t SaveFlag;        // 存储触发标志
extern uint8_t ToggleSaveFlag;  // 自动存储开关
```

### 菜单状态机模式
- `Menu1()`: 主菜单，返回选中项编号 (1-4)
- `Menu2_XXX()`: 子菜单，返回 0 表示返回上级
- 使用 `Key_Check(KEY_X, EVENT)` 实现防抖和长按检测 (见 `Key.c`)

### W25Q64 存储策略
- **循环写入**: 记录序号 1~`MaxRecordCount` (默认10)，满时 `ToggleSaveFlag=0` 停止
- **扇区管理**: 每条记录 13 字节 (`SensorData` 结构体)，按需擦除 4KB 扇区
- **数据验证**: 写入后使用 `memcmp()` 验证数据完整性（见 `DataStorage.c:85`）

### LCD 显示惯例
- 坐标系: (0,0) 左上角，像素为单位
- 中文显示: `LCD_ShowChinese(x, y, "文本", LCD_16X16, 前景色, 背景色, 清除标志)`
- 数字显示: `LCD_ShowNum(x, y, 数值, LCD_8X16, 前景, 背景, 0, 填充, 位数)`
- 颜色定义在 `LCD.h` (如 `BLUE2 0x1c9f`, `WHITE 0xffff`)

## 常见任务

### 添加新传感器
1. 在 `Hardware/` 创建驱动文件 (参考 `TempSensor.c/h`)
2. 实现初始化函数和数据读取函数
3. 在 `main.c` 初始化序列中调用
4. 在 `Menu.c` 对应显示函数中读取数据

### 修改定时器周期
TIM2 (1秒周期) 计算: `(PSC+1)*(ARR+1)/72MHz = 7200*10000/72000000 = 1s`  
修改 `Timer.c` 中的 `TIM_Prescaler` 或 `TIM_Period` 值

### 扩展存储容量
修改 `DataStorage.h` 中的 `MaxRecordCount`（实际通过菜单设置保存在 `0x001000` 地址）

### 调整音乐播放
- 音符数据在 `BuzzerData.c` 中定义为二维数组
- 频率表 `Buzzer_Freq[]` 映射音符到 PWM 预分频值
- 使用 `Buzzer_Play()` 在主循环中调用（状态机模式，非阻塞）

## 关键文件说明

- **`Menu.c` (2349行)**: 核心 UI 逻辑，包含所有菜单和 TIM2/TIM4 中断处理
- **`DataStorage.c`**: W25Q64 记录管理（循环队列式写入）
- **`main.c`**: 初始化序列和主循环菜单调度
- **`Timer.c`**: 定时器初始化（中断处理在 `Menu.c` 中覆盖）
- **`Key.c`**: 状态机式按键检测（支持单击/双击/长按/连发）

## 调试技巧
- 串口未启用，使用 LED (`LED.c`) 或 LCD 输出调试信息
- W25Q64 写入失败时检查扇区擦除状态（`Index == 0xFF` 表示未初始化）
- 定时器中断冲突时检查 `Menu.c` 和 `Timer.c` 的 `IRQHandler` 定义
- 编译错误时检查头文件路径配置在 `.eide/eide.yml` 的 `incList` 中
