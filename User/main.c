#include "Menu.h"
#include "Timer.h"
#include "Encoder.h"

uint8_t PageFlag;
extern uint8_t ToggleSaveFlag;
extern uint8_t SaveFlag;
extern float Temp;
extern uint16_t Light;
extern DateTime_New CurrentTime;

int main()
{
	// NVIC中断优先级分组配置(全局仅需配置一次)
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); // 分组2: 抢占优先级0~3, 响应优先级0~3

	// 基础外设初始化
	LCD_Init(); // LCD初始化(包含SPI1初始化)

	Key_Init();	   // 按键初始化
	Buzzer_Init(); // 蜂鸣器初始化(包含TIM3和PWM初始化)
	Buzzer_OFF();

	// 传感器初始化
	DS18B20_Init(); // 温度传感器初始化
	LDR_Init();		// 光照传感器初始化(包含ADC初始化)

	// 时间和存储初始化
	MyRTC_Init();			// RTC初始化
	DataStorage_New_Init(); // 数据存储初始化(包含W25Q64和SPI2初始化)
	// W25Q64_ChipErase(); // 擦除整个芯片（仅测试用，实际应用可注释掉）

	// 编码器初始化(必须在Timer_Init之前，避免TIM4冲突)
	//Encoder_Init(); // EC11编码器初始化(占用TIM4编码器模式)

	// 定时器初始化(放在最后,因为会立即启动中断)
	Timer_Init(); // TIM2初始化并启动定时中断(TIM4已被编码器占用)
	LED_Init();	  // LED初始化
	LED_ON();

	while (1)
	{
		PageFlag = Menu1();
		if (PageFlag == 1)
		{
			Menu2_Stats();
		}
		if (PageFlag == 2)
		{
			Menu2_History();
		}
		if (PageFlag == 3)
		{
			Menu2_Function();
		}
		if (PageFlag == 4)
		{
			Menu2_Setting();
		}
		// 主菜单中的自动存储处理
		if (SaveFlag == 1 && ToggleSaveFlag == 1)
		{
			SaveFlag = 0;
			MyRTC_ReadTime();
			CurrentTime.Year = MyRTC_Time[0];
			CurrentTime.Month = MyRTC_Time[1];
			CurrentTime.Day = MyRTC_Time[2];
			CurrentTime.Hour = MyRTC_Time[3];
			CurrentTime.Minute = MyRTC_Time[4];
			CurrentTime.Second = MyRTC_Time[5];
			Temp = DS18B20_Get_Temp();
			Light = LDR_LuxData();
			LCD_ShowChinese(80, 140, "存储中", LCD_16X16, BLUE2, WHITE, 1);
			uint8_t saveResult = DataStorage_New_Save(Temp, Light, &CurrentTime);
			if (saveResult == 1)
			{
				LCD_ShowChinese(80, 140, " 已满 ", LCD_16X16, RED, WHITE, 1);
				ToggleSaveFlag = 0; // 存储已满
			}
		}
	}
}
