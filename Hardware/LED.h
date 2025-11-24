#ifndef __LED_H
#define __LED_H
#include "stm32f10x.h"

#define LED_OB_PORT GPIOC
#define LED_OB_CLK RCC_APB2Periph_GPIOC
#define LED_OB_PIN GPIO_Pin_13

void LED_Init(void);
void LED_ON(void);
void LED_OFF(void);
void LED_Turn(void);
#endif
