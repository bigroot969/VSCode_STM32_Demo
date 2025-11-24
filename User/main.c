#include "stm32f10x.h"
#include "stdio.h"
#include "LED.h"
#include "Delay.h"
#include "LightSensor.h"
#include "LCD.h"
#include "Timer.h"
#include "Key.h"
#include "PWM.h"
#include "Buzzer.h"
#include "ADCx.h"
#include "TempSensor.h"
#include "MyRTC.h"
#include "Menu.h"
#include "DataStorage.h"

uint8_t PageFlag;
extern uint8_t ToggleSaveFlag;
extern uint8_t SaveFlag;
extern float Temp;
extern uint16_t Light;
extern DateTime CurrentTime;

int main()
{
	// 系统配置(必须在所有中断初始化之前)
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); // 配置NVIC中断优先级分组

	// 基础外设初始化
	LCD_Init(); // LCD初始化(包含SPI1初始化)

	Key_Init();	   // 按键初始化
	Buzzer_Init(); // 蜂鸣器初始化(包含TIM3和PWM初始化)
	Buzzer_OFF();

	// 传感器初始化
	DS18B20_Init(); // 温度传感器初始化
	LDR_Init();		// 光照传感器初始化(包含ADC初始化)

	// 时间和存储初始化
	MyRTC_Init(); // RTC初始化
	DataStorage_Init(); // 数据存储初始化(包含W25Q64和SPI2初始化)

	// 定时器初始化(放在最后,因为会立即启动中断)
	Timer_Init(); // TIM2/TIM4初始化并启动定时中断
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
			uint8_t saveResult = DataStorage_Save(Temp, Light, &CurrentTime);
			if (saveResult == 1)
			{
				LCD_ShowChinese(80, 140, " 已满 ", LCD_16X16, RED, WHITE, 1);
				ToggleSaveFlag = 0; // 存储已满
			}
		}
	}
}
