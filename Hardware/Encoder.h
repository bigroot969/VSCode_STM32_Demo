#ifndef __ENCODER_H
#define __ENCODER_H

#include "stm32f10x.h"

// 编码器事件标志位
#define ENCODER_CW          0x01    // 顺时针
#define ENCODER_CCW         0x02    // 逆时针
#define ENCODER_BTN_SINGLE  0x04    // 单击
#define ENCODER_BTN_LONG    0x08    // 长按

// 兼容旧代码的枚举（可选，如果不再使用可删除）
typedef enum
{
    ENCODER_DIR_NONE = 0,
    ENCODER_DIR_CW = 1,
    ENCODER_DIR_CCW = -1
} Encoder_Direction;

typedef enum
{
    ENCODER_BTN_NONE = 0,
    ENCODER_BTN_SINGLE_ENUM = 1,
    ENCODER_BTN_LONG_ENUM = 2,
    ENCODER_BTN_PRESS_ENUM = 3
} Encoder_ButtonState;

// 初始化编码器
void Encoder_Init(void);

// 1ms定时器调用
void Encoder_Tick(void);

// 检查编码器事件（类似Key_Check，读取后自动清除对应标志）
uint8_t Encoder_Check(uint8_t Flag);

// 获取旋转方向（旧接口兼容）
Encoder_Direction Encoder_GetDirection(void);

// 获取按键状态（旧接口兼容）
Encoder_ButtonState Encoder_GetButton(void);

// 定时器中断中调用（用于按键防抖和长按检测）
void Encoder_Tick(void);

#endif



