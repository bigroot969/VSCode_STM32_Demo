#ifndef __IWDG_H
#define __IWDG_H

#include "stm32f10x.h"

/**
 * @brief  初始化独立看门狗 (IWDG)
 * @param  timeout_ms: 超时时间(毫秒), 范围: 1~26214ms
 * @retval None
 * @note   IWDG时钟源为LSI (40kHz)
 *         一旦启动后无法停止,只能通过复位来关闭
 *         适用范围: 1ms ~ 26214ms (使用预分频256时)
 */
void IWDG_Init(uint16_t timeout_ms);

/**
 * @brief  喂狗 (重载看门狗计数器)
 * @param  None
 * @retval None
 * @note   必须在超时时间内周期性调用此函数,否则系统复位
 */
void IWDG_Feed(void);

#endif /* __IWDG_H */
