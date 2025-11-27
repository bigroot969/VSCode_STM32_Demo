#include "Encoder.h"

// 编码器状态变量
static uint8_t encoder_A_Prev = 0;
static uint8_t encoder_B_Prev = 0;
static int8_t encoder_Count = 0;
static uint8_t Encoder_EventFlag = 0; // 事件标志位

// 按键状态变量
static uint16_t btn_Count = 0;
static uint8_t btn_State = 0; // 0:松开, 1:按下
static uint8_t btn_LongPressReported = 0;

#define ENCODER_PULSE_PER_STEP 4   // 每格脉冲数，根据手感调整(2或4)
#define BTN_DEBOUNCE_MS 10         // 消抖时间(ms)
#define BTN_LONG_PRESS_MS 800      // 长按判定时间(ms)

/**
 * @brief 初始化EC11编码器
 * 引脚连接:
 * S1 (A) -> PB6
 * S2 (B) -> PB7
 * KEY    -> PB8
 * GND    -> GND
 * 5V     -> 3.3V (可选，用于上拉)
 */
void Encoder_Init(void)
{
    // 1. 使能GPIOB时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    // 2. 配置PB6(A), PB7(B), PB8(KEY)为上拉输入
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 启用内部上拉，适应无外部上拉的模块
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // 初始化状态
    encoder_A_Prev = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_6);
    encoder_B_Prev = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7);
}

/**
 * @brief 检查编码器事件（类似Key_Check）
 * @param Flag 要检查的事件标志 (ENCODER_CW, ENCODER_CCW, ENCODER_BTN_SINGLE, ENCODER_BTN_LONG)
 * @return 1:事件发生, 0:未发生
 */
uint8_t Encoder_Check(uint8_t Flag)
{
    if (Encoder_EventFlag & Flag)
    {
        Encoder_EventFlag &= ~Flag; // 清除标志
        return 1;
    }
    return 0;
}

/**
 * @brief 获取旋转方向（兼容旧接口）
 */
Encoder_Direction Encoder_GetDirection(void)
{
    if (Encoder_Check(ENCODER_CW)) return ENCODER_DIR_CW;
    if (Encoder_Check(ENCODER_CCW)) return ENCODER_DIR_CCW;
    return ENCODER_DIR_NONE;
}

/**
 * @brief 获取按键事件（兼容旧接口）
 */
Encoder_ButtonState Encoder_GetButton(void)
{
    if (Encoder_Check(ENCODER_BTN_SINGLE)) return ENCODER_BTN_SINGLE_ENUM;
    if (Encoder_Check(ENCODER_BTN_LONG)) return ENCODER_BTN_LONG_ENUM;
    return ENCODER_BTN_NONE;
}

/**
 * @brief 1ms定时器调用，处理编码器和按键
 */
void Encoder_Tick(void)
{
    // --- 编码器旋转检测 (双边沿检测) ---
    uint8_t A = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_6);
    uint8_t B = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7);

    if (A != encoder_A_Prev) // A相电平变化
    {
        if (A == 0) // A下降沿
        {
            if (B == 0) encoder_Count++; // 顺时针? (取决于接线)
            else encoder_Count--;
        }
        else // A上升沿
        {
            if (B == 1) encoder_Count++;
            else encoder_Count--;
        }
    }
    if (B != encoder_B_Prev) // B相电平变化
    {
        if (B == 0) // B下降沿
        {
            if (A == 1) encoder_Count++;
            else encoder_Count--;
        }
        else // B上升沿
        {
            if (A == 0) encoder_Count++;
            else encoder_Count--;
        }
    }
    
    encoder_A_Prev = A;
    encoder_B_Prev = B;

    // 判定旋转步数
    if (encoder_Count >= ENCODER_PULSE_PER_STEP)
    {
        Encoder_EventFlag |= ENCODER_CW; // 触发顺时针
        encoder_Count = 0;
    }
    else if (encoder_Count <= -ENCODER_PULSE_PER_STEP)
    {
        Encoder_EventFlag |= ENCODER_CCW; // 触发逆时针
        encoder_Count = 0;
    }

    // --- 按键检测 (状态机) ---
    uint8_t key_phys = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_8); // 0=按下, 1=松开

    if (key_phys == 0) // 按下
    {
        if (btn_State == 0) // 刚按下
        {
            btn_Count++;
            if (btn_Count >= BTN_DEBOUNCE_MS)
            {
                btn_State = 1; // 确认按下
                btn_Count = 0;
                btn_LongPressReported = 0;
            }
        }
        else // 持续按下
        {
            btn_Count++;
            if (btn_Count >= BTN_LONG_PRESS_MS && !btn_LongPressReported)
            {
                Encoder_EventFlag |= ENCODER_BTN_LONG; // 触发长按
                btn_LongPressReported = 1;
            }
        }
    }
    else // 松开
    {
        if (btn_State == 1) // 刚松开
        {
            // 如果没有触发过长按，则触发单击
            if (!btn_LongPressReported)
            {
                Encoder_EventFlag |= ENCODER_BTN_SINGLE; // 触发单击
            }
            btn_State = 0;
            btn_Count = 0;
        }
        else // 持续松开
        {
            btn_Count = 0;
        }
    }
}

// 移除 EXTI 中断函数，避免冲突
void EXTI9_5_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line8) == SET)
    {
        EXTI_ClearITPendingBit(EXTI_Line8);
    }
}
