#include "stm32f10x.h" // Device header

/**
 * 函    数：定时中断初始化
 * 参    数：无
 * 返 回 值：无
 */
void Timer_Init(void)
{
	/*开启时钟*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); // 开启TIM2的时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	/*配置时钟源*/
	TIM_InternalClockConfig(TIM2); // 选择TIM2为内部时钟，若不调用此函数，TIM默认也为内部时钟
	TIM_InternalClockConfig(TIM4);
	/*TIM2时基单元初始化*/
	TIM_TimeBaseInitTypeDef TIM2_TimeBaseInitStructure;				 // 定义结构体变量
	TIM2_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;	 // 时钟分频，选择不分频，此参数用于配置滤波器时钟，不影响时基单元功能
	TIM2_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; // 计数器模式，选择向上计数
	TIM2_TimeBaseInitStructure.TIM_Period = 10000 - 1;				 // 计数周期，即ARR的值
	TIM2_TimeBaseInitStructure.TIM_Prescaler = 7200 - 1;			 // 预分频器，即PSC的值
	TIM2_TimeBaseInitStructure.TIM_RepetitionCounter = 0;			 // 重复计数器，高级定时器才会用到
	TIM_TimeBaseInit(TIM2, &TIM2_TimeBaseInitStructure);			 // 将结构体变量交给TIM_TimeBaseInit，配置TIM2的时基单元

	/*TIM4时基单元配置*/
	TIM_TimeBaseInitTypeDef TIM4_TimeBaseInitStructure; // TIM4专用结构体
	TIM4_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM4_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM4_TimeBaseInitStructure.TIM_Period = 100 - 1;	// ARR值
	TIM4_TimeBaseInitStructure.TIM_Prescaler = 720 - 1; // PSC预分频
	TIM4_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM4, &TIM4_TimeBaseInitStructure);

	/*中断输出配置*/
	TIM_ClearFlag(TIM2, TIM_FLAG_Update); // 清除定时器更新标志位
										  // TIM_TimeBaseInit函数末尾，手动产生了更新事件
										  // 若不清除此标志位，则开启中断后，会立刻进入一次中断
										  // 如果不介意此问题，则不清除此标志位也可

	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE); // 开启TIM2的更新中断

	TIM_ClearFlag(TIM4, TIM_FLAG_Update);
	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);

	/*NVIC配置(注意：NVIC分组应在main函数中统一配置)*/
	NVIC_InitTypeDef NVIC2_InitStructure;					   // 定义结构体变量
	NVIC2_InitStructure.NVIC_IRQChannel = TIM2_IRQn;		   // 选择配置NVIC的TIM2线
	NVIC2_InitStructure.NVIC_IRQChannelCmd = ENABLE;		   // 指定NVIC线路使能
	NVIC2_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; // 指定NVIC线路的抢占优先级为2
	NVIC2_InitStructure.NVIC_IRQChannelSubPriority = 1;		   // 指定NVIC线路的响应优先级为1
	NVIC_Init(&NVIC2_InitStructure);						   // 将结构体变量交给NVIC_Init，配置NVIC外设

	NVIC_InitTypeDef NVIC4_InitStructure;
	NVIC4_InitStructure.NVIC_IRQChannel = TIM4_IRQn; // TIM4中断通道
	NVIC4_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC4_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; // 抢占优先级2（与TIM2相同）
	NVIC4_InitStructure.NVIC_IRQChannelSubPriority = 2;		   // 响应优先级2（低于TIM2）
	NVIC_Init(&NVIC4_InitStructure);

	/*TIM使能*/
	TIM_Cmd(TIM2, ENABLE); // 使能TIM2，定时器开始运行
	TIM_Cmd(TIM4, ENABLE);
}

/* 定时器中断函数，可以复制到使用它的地方
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{

		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}
*/
