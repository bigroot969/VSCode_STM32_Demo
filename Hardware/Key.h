#ifndef __KEY_H
#define __KEY_H
#include "stm32f10x.h" // Device header

#define Key_Clk RCC_APB2Periph_GPIOA
#define Key_GPIO GPIOA
#define Key1_Pin GPIO_Pin_8
#define Key2_Pin GPIO_Pin_9
#define Key3_Pin GPIO_Pin_10
#define Key4_Pin GPIO_Pin_11

#define KEY_COUNT 4

#define KEY_1 0
#define KEY_2 1
#define KEY_3 2
#define KEY_4 3

#define KEY_HOLD 0x01
#define KEY_DOWN 0x02
#define KEY_UP 0x04
#define KEY_SINGLE 0x08
#define KEY_DOUBLE 0x10
#define KEY_LONG 0x20
#define KEY_REPEAT 0x40

void Key_Init(void);
uint8_t Key_Check(uint8_t n, uint8_t Flag);
void Key_Tick(void);
#endif
