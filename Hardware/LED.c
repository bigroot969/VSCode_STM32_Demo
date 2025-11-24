#include "stm32f10x.h" // Device header
#include "LED.h"
/**
 * 函    数：LED初始化
 * 参    数：无
 * 返 回 值：无
 */
void LED_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(LED_OB_CLK, ENABLE); // 开启GPIOC的时钟

	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = LED_OB_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(LED_OB_PORT, &GPIO_InitStructure); // 将PC13引脚初始化为推挽输出

	/*设置GPIO初始化后的默认电平*/
	GPIO_SetBits(LED_OB_PORT, LED_OB_PIN); // 设置PC13引脚为高电平
}

/**
 * 函    数：LED开启
 * 参    数：无
 * 返 回 值：无
 */
void LED_ON(void)
{
	GPIO_ResetBits(LED_OB_PORT, LED_OB_PIN); // 设置PC13引脚为低电平
}

/**
 * 函    数：LED关闭
 * 参    数：无
 * 返 回 值：无
 */
void LED_OFF(void)
{
	GPIO_SetBits(LED_OB_PORT, LED_OB_PIN); // 设置PC13引脚为高电平
}

/**
 * 函    数：LED状态翻转
 * 参    数：无
 * 返 回 值：无
 */
void LED_Turn(void)
{
	if (GPIO_ReadOutputDataBit(LED_OB_PORT, LED_OB_PIN) == 0) // 获取输出寄存器的状态，如果当前引脚输出低电平
	{
		GPIO_SetBits(LED_OB_PORT, LED_OB_PIN); // 则设置PC13引脚为高电平
	}
	else // 否则，即当前引脚输出高电平
	{
		GPIO_ResetBits(LED_OB_PORT, LED_OB_PIN); // 则设置PC13引脚为低电平
	}
}
