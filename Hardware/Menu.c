#include "stm32f10x.h" // Device header
#include "LCD.h"
#include "Key.h"
#include "Menu.h"
#include "MyRTC.h"
#include "LightSensor.h"
#include "TempSensor.h"
#include "Buzzer.h"
#include "DataStorage.h"
#include "stdio.h"

#define Music_Count 5

uint8_t Music = 1;

extern int MyRTC_Time[];
int SaveInterval_Time[3];
extern uint8_t MaxRecordCount;
DateTime CurrentTime;
int SaveInterval = 10;
uint8_t StopWatchStartFlag = 0;
uint8_t hour, minute, second;
uint8_t Flag = 1;
uint8_t StepLength = 5;
uint16_t LightValue = 400;
int TempValue = 35;
uint8_t ToggleSaveFlag = 0;
uint8_t NowTotalRecords;
uint8_t ShowRecordID = 0;
float Temp = 0;
uint16_t Light = 0;
volatile uint8_t SaveFlag = 0;
volatile int TimerCount = 0;

int Menu1(void) // 一级菜单
{
	LCD_Clear(WHITE);
	LCD_ShowChinese(0, 32, "状态信息", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowChinese(0, 56, "历史数据", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowChinese(0, 80, "功能", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowChinese(0, 104, "设置", LCD_16X16, BLACK, WHITE, 1);
	while (1)
	{
		if (Key_Check(KEY_1, KEY_UP) || Key_Check(KEY_1, KEY_REPEAT))
		{
			Flag--;
			if (Flag == 0)
			{
				Flag = 4;
			}
		}
		if (Key_Check(KEY_2, KEY_UP) || Key_Check(KEY_2, KEY_REPEAT))
		{
			Flag++;
			if (Flag == 5)
			{
				Flag = 1;
			}
		}
		if (Key_Check(KEY_3, KEY_SINGLE))
		{
			LCD_Clear(WHITE);
			return Flag;
		}
		switch (Flag)
		{
		case 1:
		{
			LCD_ShowChinese(0, 32, "状态信息", LCD_16X16, WHITE, BLACK, 1);
			LCD_ShowChinese(0, 56, "历史数据", LCD_16X16, BLACK, WHITE, 1);
			LCD_ShowChinese(0, 80, "功能", LCD_16X16, BLACK, WHITE, 1);
			LCD_ShowChinese(0, 104, "设置", LCD_16X16, BLACK, WHITE, 1);
			break;
		}
		case 2:
		{
			LCD_ShowChinese(0, 32, "状态信息", LCD_16X16, BLACK, WHITE, 1);
			LCD_ShowChinese(0, 56, "历史数据", LCD_16X16, WHITE, BLACK, 1);
			LCD_ShowChinese(0, 80, "功能", LCD_16X16, BLACK, WHITE, 1);
			LCD_ShowChinese(0, 104, "设置", LCD_16X16, BLACK, WHITE, 1);
			break;
		}
		case 3:
		{
			LCD_ShowChinese(0, 32, "状态信息", LCD_16X16, BLACK, WHITE, 1);
			LCD_ShowChinese(0, 56, "历史数据", LCD_16X16, BLACK, WHITE, 1);
			LCD_ShowChinese(0, 80, "功能", LCD_16X16, WHITE, BLACK, 1);
			LCD_ShowChinese(0, 104, "设置", LCD_16X16, BLACK, WHITE, 1);
			break;
		}
		case 4:
		{
			LCD_ShowChinese(0, 32, "状态信息", LCD_16X16, BLACK, WHITE, 1);
			LCD_ShowChinese(0, 56, "历史数据", LCD_16X16, BLACK, WHITE, 1);
			LCD_ShowChinese(0, 80, "功能", LCD_16X16, BLACK, WHITE, 1);
			LCD_ShowChinese(0, 104, "设置", LCD_16X16, WHITE, BLACK, 1);
			break;
		}
		}
	}
}

int Menu2_Stats(void) // 二级菜单
{
	uint8_t StFlag = 1;
	uint8_t Menu2_StatsFlag = 0;
	uint8_t AlarmActiveFlag = 0;
	uint8_t AlarmCounter = 0;
	uint8_t LastAlarmState = 0; // 上次报警状态，用于检测状态变化
	const uint16_t MAX_ALARM_TICKS = 5;

	LCD_ShowChinese(0, 0, "状态信息", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowChinese(0, 20, "<---", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 80, "光照(lux):", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 40, "日期:", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 60, "时间:", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 100, "温度(C):", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowString(72, 40, "-", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowString(96, 40, "-", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowString(56, 60, ":", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowString(80, 60, ":", LCD_8X16, BLACK, WHITE, 0);

	while (1)
	{
		if (Key_Check(KEY_3, KEY_SINGLE))
		{
			LCD_Clear(WHITE);
			Buzzer_OFF();
			Menu2_StatsFlag = StFlag;
		}
		if (Menu2_StatsFlag == 1)
		{
			return 0;
		}
		switch (StFlag)
		{
		case 1:
		{
			LCD_ShowString(0, 20, "<---", LCD_8X16, WHITE, BLACK, 0);
			break;
		}
		}

		// 读取传感器数据
		MyRTC_ReadTime();
		Light = LDR_LuxData();
		Temp = Temp_Average_Data();

		// 显示时间
		LCD_ShowNum(40, 40, MyRTC_Time[0], LCD_8X16, BLACK, WHITE, 0, 0, 4);  // 年
		LCD_ShowNum(80, 40, MyRTC_Time[1], LCD_8X16, BLACK, WHITE, 0, 1, 2);  // 月
		LCD_ShowNum(104, 40, MyRTC_Time[2], LCD_8X16, BLACK, WHITE, 0, 1, 2); // 日
		LCD_ShowNum(40, 60, MyRTC_Time[3], LCD_8X16, BLACK, WHITE, 0, 1, 2);  // 时
		LCD_ShowNum(64, 60, MyRTC_Time[4], LCD_8X16, BLACK, WHITE, 0, 1, 2);  // 分
		LCD_ShowNum(88, 60, MyRTC_Time[5], LCD_8X16, BLACK, WHITE, 0, 1, 2);  // 秒

		// 自动存储处理
		if (SaveFlag == 1 && ToggleSaveFlag == 1)
		{
			SaveFlag = 0;
			CurrentTime.Year = MyRTC_Time[0];
			CurrentTime.Month = MyRTC_Time[1];
			CurrentTime.Day = MyRTC_Time[2];
			CurrentTime.Hour = MyRTC_Time[3];
			CurrentTime.Minute = MyRTC_Time[4];
			CurrentTime.Second = MyRTC_Time[5];
			// 读取实时传感器数据
			float tempData = DS18B20_Get_Temp();
			uint16_t lightData = LDR_LuxData();
			uint8_t saveResult = DataStorage_Save(tempData, lightData, &CurrentTime);
			if (saveResult == 1)
			{
				ToggleSaveFlag = 0; // 存储已满
			}
		}

		// 持续显示存储状态
		if (ToggleSaveFlag == 1)
		{
			LCD_ShowChinese(80, 140, "存储中", LCD_16X16, BLUE, WHITE, 1);
		}
		else
		{
			NowTotalRecords = DataStorage_GetCount();
			if (NowTotalRecords >= MaxRecordCount)
			{
				LCD_ShowChinese(80, 140, " 已满 ", LCD_16X16, RED, WHITE, 1);
			}
		}

		// 判断是否需要报警
		uint8_t CurrentAlarmState = (Temp >= TempValue || Light >= LightValue) ? 1 : 0;

		// 只在状态变化时更新中文标签，减少闪烁
		if (CurrentAlarmState != LastAlarmState)
		{
			if (CurrentAlarmState)
			{
				LCD_ShowChinese(0, 80, "光照(lux):", LCD_16X16, RED, WHITE, 0);
				LCD_ShowChinese(0, 100, "温度(C):", LCD_16X16, RED, WHITE, 0);
			}
			else
			{
				LCD_ShowChinese(0, 80, "光照(lux):", LCD_16X16, BLACK, WHITE, 0);
				LCD_ShowChinese(0, 100, "温度(C):", LCD_16X16, BLACK, WHITE, 0);
			}
			LastAlarmState = CurrentAlarmState;
		}

		// 显示数值（使用当前状态的颜色）
		uint16_t displayColor = CurrentAlarmState ? RED : BLACK;
		LCD_ShowNum(88, 80, Light, LCD_8X16, displayColor, WHITE, 0, 1, 3);
		// 温度显示，FixedWidth=6可容纳"-99.9"格式
		LCD_ShowFloatNum(72, 100, Temp, 1, LCD_8X16, displayColor, WHITE, 0, 6);

		// 报警逻辑处理
		if (CurrentAlarmState)
		{
			if (!AlarmActiveFlag)
			{
				Buzzer_ON();
				AlarmActiveFlag = 1;
				AlarmCounter = 0;
			}
			else
			{
				AlarmCounter++;
				if (AlarmCounter >= MAX_ALARM_TICKS)
				{
					Buzzer_OFF();
					AlarmActiveFlag = 0;
				}
			}
		}
		else
		{
			if (AlarmActiveFlag)
			{
				Buzzer_OFF();
				AlarmActiveFlag = 0;
				AlarmCounter = 0;
			}
		}
	}
}

int Menu2_Setting(void)
{
	uint8_t Menu3_Set = 0;
	uint8_t SetFlag = 1;
	LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 40, "日期时间", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 60, "光照阈值", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 80, "温度阈值", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 100, "单次步长", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 0, "设置", LCD_16X16, BLACK, WHITE, 1);
	while (1)
	{
		// 自动存储处理（读取实时传感器数据）
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
			// 读取实时传感器数据
			float tempData = DS18B20_Get_Temp();
			uint16_t lightData = LDR_LuxData();
			uint8_t saveResult = DataStorage_Save(tempData, lightData, &CurrentTime);
			if (saveResult == 1)
			{
				ToggleSaveFlag = 0; // 存储已满
			}
		}

		// 持续显示存储状态
		if (ToggleSaveFlag == 1)
		{
			LCD_ShowChinese(80, 140, "存储中", LCD_16X16, BLUE, WHITE, 1);
		}
		else
		{
			NowTotalRecords = DataStorage_GetCount();
			if (NowTotalRecords >= MaxRecordCount)
			{
				LCD_ShowChinese(80, 140, " 已满 ", LCD_16X16, RED, WHITE, 1);
			}
		}

		if (Key_Check(KEY_1, KEY_UP) || Key_Check(KEY_1, KEY_REPEAT))
		{
			SetFlag--;
			if (SetFlag == 0)
			{
				SetFlag = 5;
			}
		}
		if (Key_Check(KEY_2, KEY_UP) || Key_Check(KEY_2, KEY_REPEAT))
		{
			SetFlag++;
			if (SetFlag == 6)
			{
				SetFlag = 1;
			}
		}
		if (Key_Check(KEY_3, KEY_SINGLE))
		{
			LCD_Clear(WHITE);
			Menu3_Set = SetFlag;
		}
		if (Menu3_Set == 1)
		{
			return 0;
		}
		if (Menu3_Set == 2)
		{
			Menu3_Set = Menu3_SetDate();
		}
		if (Menu3_Set == 3)
		{
			Menu3_Set = Menu3_SetLight();
		}
		if (Menu3_Set == 4)
		{
			Menu3_Set = Menu3_SetTemp();
		}
		if (Menu3_Set == 5)
		{
			Menu3_Set = Menu3_SetLength();
		}
		switch (SetFlag)
		{
		case 1:
		{
			LCD_ShowString(0, 20, "<---", LCD_8X16, WHITE, BLACK, 0);
			LCD_ShowChinese(0, 40, "日期时间", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 60, "光照阈值", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 80, "温度阈值", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 100, "单次步长", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowNum(72, 100, StepLength, LCD_8X16, BLACK, WHITE, 0, 0, 0);
			LCD_ShowNum(72, 60, LightValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
			LCD_ShowNum(72, 80, TempValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
			LCD_ShowChinese(0, 0, "设置", LCD_16X16, BLACK, WHITE, 1);
			break;
		}
		case 2:
		{
			LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 40, "日期时间", LCD_16X16, WHITE, BLACK, 0);
			LCD_ShowChinese(0, 60, "光照阈值", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 80, "温度阈值", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 100, "单次步长", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowNum(72, 100, StepLength, LCD_8X16, BLACK, WHITE, 0, 0, 0);
			LCD_ShowNum(72, 60, LightValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
			LCD_ShowNum(72, 80, TempValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
			LCD_ShowChinese(0, 0, "设置", LCD_16X16, BLACK, WHITE, 1);
			break;
		}
		case 3:
		{
			LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 40, "日期时间", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 60, "光照阈值", LCD_16X16, WHITE, BLACK, 0);
			LCD_ShowChinese(0, 80, "温度阈值", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 100, "单次步长", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowNum(72, 100, StepLength, LCD_8X16, BLACK, WHITE, 0, 0, 0);
			LCD_ShowNum(72, 60, LightValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
			LCD_ShowNum(72, 80, TempValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
			LCD_ShowChinese(0, 0, "设置", LCD_16X16, BLACK, WHITE, 1);
			break;
		}
		case 4:
		{
			LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 40, "日期时间", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 60, "光照阈值", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 80, "温度阈值", LCD_16X16, WHITE, BLACK, 0);
			LCD_ShowChinese(0, 100, "单次步长", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowNum(72, 100, StepLength, LCD_8X16, BLACK, WHITE, 0, 0, 0);
			LCD_ShowNum(72, 60, LightValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
			LCD_ShowNum(72, 80, TempValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
			LCD_ShowChinese(0, 0, "设置", LCD_16X16, BLACK, WHITE, 1);
			break;
		}
		case 5:
		{
			LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 40, "日期时间", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 60, "光照阈值", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 80, "温度阈值", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 100, "单次步长", LCD_16X16, WHITE, BLACK, 0);
			LCD_ShowNum(72, 100, StepLength, LCD_8X16, BLACK, WHITE, 0, 0, 0);
			LCD_ShowNum(72, 60, LightValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
			LCD_ShowNum(72, 80, TempValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
			LCD_ShowChinese(0, 0, "设置", LCD_16X16, BLACK, WHITE, 1);
			break;
		}
		}
	}
}

int Menu2_Function(void)
{
	uint8_t FunctionFlag = 1;
	uint8_t Menu2_Finction = 0;
	LCD_ShowChinese(0, 0, "功能", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 40, "音乐播放", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 60, "秒表", LCD_16X16, BLACK, WHITE, 0);
	while (1)
	{
		// 自动存储处理（读取实时传感器数据）
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
			// 读取实时传感器数据
			float tempData = DS18B20_Get_Temp();
			uint16_t lightData = LDR_LuxData();
			uint8_t saveResult = DataStorage_Save(tempData, lightData, &CurrentTime);
			if (saveResult == 1)
			{
				ToggleSaveFlag = 0; // 存储已满
			}
		}

		// 持续显示存储状态
		if (ToggleSaveFlag == 1)
		{
			LCD_ShowChinese(80, 140, "存储中", LCD_16X16, BLUE, WHITE, 1);
		}
		else
		{
			NowTotalRecords = DataStorage_GetCount();
			if (NowTotalRecords >= MaxRecordCount)
			{
				LCD_ShowChinese(80, 140, " 已满 ", LCD_16X16, RED, WHITE, 1);
			}
		}

		if (Key_Check(KEY_1, KEY_UP) || Key_Check(KEY_1, KEY_REPEAT))
		{
			FunctionFlag--;
			if (FunctionFlag == 0)
			{
				FunctionFlag = 3;
			}
		}
		if (Key_Check(KEY_2, KEY_UP) || Key_Check(KEY_2, KEY_REPEAT))
		{
			FunctionFlag++;
			if (FunctionFlag == 4)
			{
				FunctionFlag = 1;
			}
		}
		if (Key_Check(KEY_3, KEY_SINGLE))
		{
			LCD_Clear(WHITE);
			Menu2_Finction = FunctionFlag;
		}
		if (Menu2_Finction == 1)
		{
			return 0;
		}
		if (Menu2_Finction == 2)
		{
			Menu2_Finction = Menu3_Music();
		}
		if (Menu2_Finction == 3)
		{
			Menu2_Finction = Menu3_StopWatch();
		}
		// if(Menu2_Finction==4){return 0;}
		switch (FunctionFlag)
		{
		case 1:
		{
			LCD_ShowString(0, 20, "<---", LCD_8X16, WHITE, BLACK, 0);
			LCD_ShowChinese(0, 40, "音乐播放", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 60, "秒表", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 0, "功能", LCD_16X16, BLACK, WHITE, 1);
			break;
		}
		case 2:
		{
			LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 40, "音乐播放", LCD_16X16, WHITE, BLACK, 0);
			LCD_ShowChinese(0, 60, "秒表", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 0, "功能", LCD_16X16, BLACK, WHITE, 1);
			break;
		}
		case 3:
		{
			LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 40, "音乐播放", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 60, "秒表", LCD_16X16, WHITE, BLACK, 0);
			LCD_ShowChinese(0, 0, "功能", LCD_16X16, BLACK, WHITE, 1);
			break;
		} /*
		 case 4:
		 {
			 LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
			 LCD_ShowChinese(0, 40, "音乐播放", LCD_16X16, BLACK, WHITE, 0);
			 LCD_ShowChinese(0, 60, "秒表", LCD_16X16, BLACK, WHITE, 0);
			 break;
		 }*/
		}
	}
}

int Menu3_Music(void)
{
	uint8_t PlayerFlag = 1;
	uint8_t CurrentMusic = 1;

	LCD_ShowChinese(0, 0, "音乐播放", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowChinese(0, 60, "当前播放", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowString(104, 40, ">>>", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 144, "暂停", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(96, 144, "退出", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 80, "小星星", LCD_16X16, BLACK, WHITE, 1);
	LCD_DrawRectangle(1, 120, 125, 126, BLACK); // 绘制进度条边框
	Buzzer_PauseFlag = 1;
	Buzzer_FinishFlag = 0;
	Buzzer_Progress = 0;
	Buzzer_Speed = BPM2Speed(MusicBPM[CurrentMusic - 1]);
	while (1)
	{
		if (Key_Check(KEY_1, KEY_UP) || Key_Check(KEY_1, KEY_REPEAT))
		{
			PlayerFlag--;
			if (PlayerFlag == 0)
			{
				PlayerFlag = 4;
			}
		}
		if (Key_Check(KEY_2, KEY_UP) || Key_Check(KEY_2, KEY_REPEAT))
		{
			PlayerFlag++;
			if (PlayerFlag == 5)
			{
				PlayerFlag = 1;
			}
		}
		if (Key_Check(KEY_3, KEY_SINGLE))
		{
			switch (PlayerFlag)
			{
			case 1:
			{
				if (Buzzer_PauseFlag || Buzzer_FinishFlag)
				{
					CurrentMusic--;
					if (CurrentMusic == 0)
					{
						CurrentMusic = Music_Count;
					}
					switch (CurrentMusic)
					{
					case 1:
						LCD_ShowChinese(0, 80, "    小星星    ", LCD_16X16, BLACK, WHITE, 1);
						break;
					case 2:
						LCD_ShowChinese(0, 80, "   天空之城   ", LCD_16X16, BLACK, WHITE, 1);
						break;
					case 3:
						LCD_ShowChinese(0, 80, "    春日影    ", LCD_16X16, BLACK, WHITE, 1);
						break;
					case 4:
						LCD_ShowChinese(0, 80, "   口琴M-7    ", LCD_16X16, BLACK, WHITE, 1);
						break;
					case 5:
						LCD_ShowString(0, 80, "Light of Nibel", LCD_8X16, BLACK, WHITE, 1);
						break;
					}
					Buzzer_Speed = BPM2Speed(MusicBPM[CurrentMusic - 1]);
					Buzzer_FinishFlag = 0;
					Buzzer_Progress = 0;
					Buzzer_PauseFlag = 1;
				}
				LCD_DrawRectangle_Fill(3, 121, 124, 125, WHITE);
				break;
			}
			case 2:
			{
				if (Buzzer_PauseFlag || Buzzer_FinishFlag)
				{
					CurrentMusic++;
					if (CurrentMusic == Music_Count + 1)
					{
						CurrentMusic = 1;
					}
					switch (CurrentMusic)
					{
					case 1:
						LCD_ShowChinese(0, 80, "    小星星    ", LCD_16X16, BLACK, WHITE, 1);
						break;
					case 2:
						LCD_ShowChinese(0, 80, "   天空之城   ", LCD_16X16, BLACK, WHITE, 1);
						break;
					case 3:
						LCD_ShowChinese(0, 80, "    春日影    ", LCD_16X16, BLACK, WHITE, 1);
						break;
					case 4:
						LCD_ShowChinese(0, 80, "   口琴M-7    ", LCD_16X16, BLACK, WHITE, 1);
						break;
					case 5:
						LCD_ShowString(0, 80, "Light of Nibel", LCD_8X16, BLACK, WHITE, 1);
						break;
					}
					Buzzer_Speed = BPM2Speed(MusicBPM[CurrentMusic - 1]);
					Buzzer_FinishFlag = 0;
					Buzzer_PauseFlag = 1;
					Buzzer_Progress = 0;
				}
				LCD_DrawRectangle_Fill(3, 121, 124, 125, WHITE);
				break;
			}
			case 3:
			{
				if (Buzzer_FinishFlag)
				{
					LCD_DrawRectangle_Fill(4, 122, 124, 124, WHITE);
					Buzzer_FinishFlag = 0;
					Buzzer_PauseFlag = 0;
					Buzzer_Progress = 0;
				}
				else
				{
					Buzzer_PauseFlag = !Buzzer_PauseFlag; // 翻转状态（核心修正）
					if (Buzzer_PauseFlag)				  // 切换到暂停
					{
						Buzzer_OFF();
					}
				}
				break;
			}
			case 4:
			{
				Buzzer_OFF();
				Buzzer_Progress = 0;
				LCD_Clear(WHITE);
				return 0;
			}
			}
		}
		switch (PlayerFlag)
		{
		case 1:
		{
			LCD_ShowString(0, 40, "<<<", LCD_8X16, WHITE, BLACK, 0);
			LCD_ShowString(104, 40, ">>>", LCD_8X16, BLACK, WHITE, 0);
			LCD_ShowChinese(96, 144, "退出", LCD_16X16, BLACK, WHITE, 0);
			if (Buzzer_PauseFlag == 1) // 如果是暂停状态
			{
				LCD_ShowChinese(0, 144, "暂停", LCD_16X16, BLACK, WHITE, 0); // 显示"PAUSE"（黑字白底）
			}
			else // 如果是播放状态
			{
				LCD_ShowChinese(0, 144, "播放", LCD_16X16, BLACK, WHITE, 0); // 显示"PLAY "（黑字白底，空格用于对齐）
			}
			break;
		}
		case 2:
		{
			LCD_ShowString(0, 40, "<<<", LCD_8X16, BLACK, WHITE, 0);
			LCD_ShowString(104, 40, ">>>", LCD_8X16, WHITE, BLACK, 0);
			LCD_ShowChinese(96, 144, "退出", LCD_16X16, BLACK, WHITE, 0);
			if (Buzzer_PauseFlag == 1) // 如果是暂停状态
			{
				LCD_ShowChinese(0, 144, "暂停", LCD_16X16, BLACK, WHITE, 0); // 显示"PAUSE"（黑字白底）
			}
			else // 如果是播放状态
			{
				LCD_ShowChinese(0, 144, "播放", LCD_16X16, BLACK, WHITE, 0); // 显示"PLAY "（黑字白底，空格用于对齐）
			}
			break;
		}
		case 3:
		{
			LCD_ShowString(0, 40, "<<<", LCD_8X16, BLACK, WHITE, 0);
			LCD_ShowString(104, 40, ">>>", LCD_8X16, BLACK, WHITE, 0);
			LCD_ShowChinese(96, 144, "退出", LCD_16X16, BLACK, WHITE, 0);
			if (Buzzer_PauseFlag == 1) // 如果是暂停状态
			{
				LCD_ShowChinese(0, 144, "暂停", LCD_16X16, WHITE, BLACK, 0); // 显示"PAUSE"（黑字白底）
			}
			else // 如果是播放状态
			{
				LCD_ShowChinese(0, 144, "播放", LCD_16X16, WHITE, BLACK, 0); // 显示"PLAY "（黑字白底，空格用于对齐）
			}
			break;
		}
		case 4:
		{
			LCD_ShowString(0, 40, "<<<", LCD_8X16, BLACK, WHITE, 0);
			LCD_ShowString(104, 40, ">>>", LCD_8X16, BLACK, WHITE, 0);
			LCD_ShowChinese(96, 144, "退出", LCD_16X16, WHITE, BLACK, 0);
			if (Buzzer_PauseFlag == 1) // 如果是暂停状态
			{
				LCD_ShowChinese(0, 144, "暂停", LCD_16X16, BLACK, WHITE, 0); // 显示"PAUSE"（黑字白底）
			}
			else // 如果是播放状态
			{
				LCD_ShowChinese(0, 144, "播放", LCD_16X16, BLACK, WHITE, 0); // 显示"PLAY "（黑字白底，空格用于对齐）
			}
			break;
		}
		}
		if (!Buzzer_PauseFlag && !Buzzer_FinishFlag)
		{
			switch (CurrentMusic)
			{
			case 1:
			{
				Buzzer_Play(LittleStar);
				break;
			}
			case 2:
			{
				Buzzer_Play(CastleInTheSky);
				break;
			}
			case 3:
			{
				Buzzer_Play(Haruhikage);
				break;
			}
			case 4:
			{
				Buzzer_Play(Orb);
				break;
			}
			case 5:
			{
				Buzzer_Play(Ori);
				break;
			}
			}
		}
		LCD_DrawRectangle_Fill(3, 122, Buzzer_Progress + 2, 124, BLUE2); // 更新进度条
	}
}

int Menu2_History(void)
{
	uint8_t Menu2_HistoryFlag = 0;
	uint8_t HisFlag = 1;
	LCD_Clear(WHITE);
	NowTotalRecords = DataStorage_GetCount();
	LCD_ShowChinese(0, 0, "历史数据", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 40, "自动存储", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 60, "读取记录", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 80, "擦除数据", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 100, "最大记录数", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 120, "当前记录数", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowNum(96, 100, MaxRecordCount, LCD_8X16, BLACK, WHITE, 0, 1, 3);
	LCD_ShowNum(96, 120, NowTotalRecords, LCD_8X16, BLACK, WHITE, 0, 1, 3);

	while (1)
	{
		// 自动存储处理（读取实时传感器数据）
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
			// 读取实时传感器数据
			float tempData = DS18B20_Get_Temp();
			uint16_t lightData = LDR_LuxData();
			uint8_t saveResult = DataStorage_Save(tempData, lightData, &CurrentTime);
			if (saveResult == 1)
			{
				ToggleSaveFlag = 0; // 存储已满
			}
			// 更新当前记录数显示
			NowTotalRecords = DataStorage_GetCount();
			LCD_ShowNum(96, 120, NowTotalRecords, LCD_8X16, BLACK, WHITE, 0, 1, 3);
		}

		// 持续显示存储状态
		if (ToggleSaveFlag == 1)
		{
			LCD_ShowChinese(80, 140, "存储中", LCD_16X16, BLUE, WHITE, 1);
		}
		else
		{
			if (NowTotalRecords >= MaxRecordCount)
			{
				LCD_ShowChinese(80, 140, " 已满 ", LCD_16X16, RED, WHITE, 1);
			}
		}

		if (Key_Check(KEY_1, KEY_UP) || Key_Check(KEY_1, KEY_REPEAT))
		{
			HisFlag--;
			if (HisFlag == 0)
			{
				HisFlag = 5;
			}
		}
		if (Key_Check(KEY_2, KEY_UP) || Key_Check(KEY_2, KEY_REPEAT))
		{
			HisFlag++;
			if (HisFlag == 6)
			{
				HisFlag = 1;
			}
		}
		if (Key_Check(KEY_3, KEY_SINGLE))
		{
			LCD_Clear(WHITE);
			Menu2_HistoryFlag = HisFlag;
		}
		if (Menu2_HistoryFlag == 1)
		{
			return 0;
		}
		if (Menu2_HistoryFlag == 2)
		{
			Menu2_HistoryFlag = Menu3_AutoSaveSet();
		}
		if (Menu2_HistoryFlag == 3)
		{
			Menu2_HistoryFlag = Menu3_ReadRecord();
		}
		if (Menu2_HistoryFlag == 4)
		{
			Menu2_HistoryFlag = Menu3_ChipErase();
		}
		if (Menu2_HistoryFlag == 5)
		{
			Menu2_HistoryFlag = Menu3_SetMaxRecord();
		}
		switch (HisFlag)
		{
		case 1:
		{
			LCD_ShowString(0, 20, "<---", LCD_8X16, WHITE, BLACK, 0);
			LCD_ShowChinese(0, 40, "自动存储", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 60, "读取记录", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 80, "擦除数据", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 100, "最大记录数", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 120, "当前记录数", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 0, "历史数据", LCD_16X16, BLACK, WHITE, 1);
			LCD_ShowNum(96, 100, MaxRecordCount, LCD_8X16, BLACK, WHITE, 0, 1, 3);
			LCD_ShowNum(96, 120, NowTotalRecords, LCD_8X16, BLACK, WHITE, 0, 1, 3);
			break;
		}
		case 2:
		{
			LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 40, "自动存储", LCD_16X16, WHITE, BLACK, 0);
			LCD_ShowChinese(0, 60, "读取记录", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 80, "擦除数据", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 100, "最大记录数", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 120, "当前记录数", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 0, "历史数据", LCD_16X16, BLACK, WHITE, 1);
			LCD_ShowNum(96, 100, MaxRecordCount, LCD_8X16, BLACK, WHITE, 0, 1, 3);
			LCD_ShowNum(96, 120, NowTotalRecords, LCD_8X16, BLACK, WHITE, 0, 1, 3);
			break;
		}
		case 3:
		{
			LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 40, "自动存储", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 60, "读取记录", LCD_16X16, WHITE, BLACK, 0);
			LCD_ShowChinese(0, 80, "擦除数据", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 100, "最大记录数", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 120, "当前记录数", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 0, "历史数据", LCD_16X16, BLACK, WHITE, 1);
			LCD_ShowNum(96, 100, MaxRecordCount, LCD_8X16, BLACK, WHITE, 0, 1, 3);
			LCD_ShowNum(96, 120, NowTotalRecords, LCD_8X16, BLACK, WHITE, 0, 1, 3);
			break;
		}
		case 4:
		{
			LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 40, "自动存储", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 60, "读取记录", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 80, "擦除数据", LCD_16X16, WHITE, BLACK, 0);
			LCD_ShowChinese(0, 100, "最大记录数", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 120, "当前记录数", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 0, "历史数据", LCD_16X16, BLACK, WHITE, 1);
			LCD_ShowNum(96, 100, MaxRecordCount, LCD_8X16, BLACK, WHITE, 0, 1, 3);
			LCD_ShowNum(96, 120, NowTotalRecords, LCD_8X16, BLACK, WHITE, 0, 1, 3);
			break;
		}
		case 5:
		{
			LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 40, "自动存储", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 60, "读取记录", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 80, "擦除数据", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 100, "最大记录数", LCD_16X16, WHITE, BLACK, 0);
			LCD_ShowChinese(0, 120, "当前记录数", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 0, "历史数据", LCD_16X16, BLACK, WHITE, 1);
			LCD_ShowNum(96, 100, MaxRecordCount, LCD_8X16, BLACK, WHITE, 0, 1, 3);
			LCD_ShowNum(96, 120, NowTotalRecords, LCD_8X16, BLACK, WHITE, 0, 1, 3);
			break;
		}
		}
	}
}

int Menu3_SetDate(void)
{
	uint8_t DateFlag = 1;
	uint8_t SwitchDateFlag = 0;  // 初始为0（切换模式）
	LCD_ShowChinese(0, 40, "日期", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 60, "时间", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 0, "设置", LCD_16X16, BLACK, WHITE, 1);
	// 显示日期时间分隔符
	LCD_ShowString(72, 40, "-", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowString(96, 40, "-", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowString(72, 60, ":", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowString(96, 60, ":", LCD_8X16, BLACK, WHITE, 0);
	// 显示日期时间，当前设置项高亮或闪烁
	LCD_ShowNum(40, 40, MyRTC_Time[0], LCD_8X16, BLACK, WHITE, 0, 0, 4);  // 年
	LCD_ShowNum(80, 40, MyRTC_Time[1], LCD_8X16, BLACK, WHITE, 0, 1, 2);  // 月
	LCD_ShowNum(104, 40, MyRTC_Time[2], LCD_8X16, BLACK, WHITE, 0, 1, 2); // 日
	LCD_ShowNum(48, 60, MyRTC_Time[3], LCD_8X16, BLACK, WHITE, 0, 1, 2);  // 时
	LCD_ShowNum(80, 60, MyRTC_Time[4], LCD_8X16, BLACK, WHITE, 0, 1, 2);  // 分
	LCD_ShowNum(104, 60, MyRTC_Time[5], LCD_8X16, BLACK, WHITE, 0, 1, 2); // 秒
	while (1)
	{
		if (SwitchDateFlag == 0)  // 切换模式
		{
			if (Key_Check(KEY_1, KEY_UP) || Key_Check(KEY_1, KEY_REPEAT))
			{
				DateFlag--;
				if (DateFlag == 0)
				{
					DateFlag = 6;
				}
			}
			else if (Key_Check(KEY_2, KEY_UP) || Key_Check(KEY_2, KEY_REPEAT))
			{
				DateFlag++;
				if (DateFlag == 7)
				{
					DateFlag = 1;
				}
			}
		}
		else if (SwitchDateFlag == 1)  // 调整模式
		{
			if (Key_Check(KEY_1, KEY_UP) || Key_Check(KEY_1, KEY_REPEAT))  // KEY_1：减少
			{
				switch (DateFlag)
				{
				case 1:  // 年份
				{
					if (MyRTC_Time[0] <= 2000)
					{
						MyRTC_Time[0] = 2099;
					}
					else
					{
						MyRTC_Time[0]--;
					}
					break;
				}
				case 2:  // 月份
				{
					if (MyRTC_Time[1] <= 1)
					{
						MyRTC_Time[1] = 12;
					}
					else
					{
						MyRTC_Time[1]--;
					}
					break;
				}
				case 3:  // 日期
				{
					if (MyRTC_Time[2] <= 1)
					{
						MyRTC_Time[2] = 31;
					}
					else
					{
						MyRTC_Time[2]--;
					}
					break;
				}
				case 4:  // 小时
				{
					if (MyRTC_Time[3] == 0)
					{
						MyRTC_Time[3] = 23;
					}
					else
					{
						MyRTC_Time[3]--;
					}
					break;
				}
				case 5:  // 分钟
				{
					if (MyRTC_Time[4] == 0)
					{
						MyRTC_Time[4] = 59;
					}
					else
					{
						MyRTC_Time[4]--;
					}
					break;
				}
				case 6:  // 秒
				{
					if (MyRTC_Time[5] == 0)
					{
						MyRTC_Time[5] = 59;
					}
					else
					{
						MyRTC_Time[5]--;
					}
					break;
				}
				}
			}
			else if (Key_Check(KEY_2, KEY_UP) || Key_Check(KEY_2, KEY_REPEAT))  // KEY_2：增加
			{
				switch (DateFlag)
				{
				case 1:  // 年份
				{
					MyRTC_Time[0]++;
					if (MyRTC_Time[0] >= 2100)
					{
						MyRTC_Time[0] = 2000;
					}
					break;
				}
				case 2:  // 月份
				{
					MyRTC_Time[1]++;
					if (MyRTC_Time[1] >= 13)
					{
						MyRTC_Time[1] = 1;
					}
					break;
				}
				case 3:  // 日期  // 日期
				{
					MyRTC_Time[2]++;
					if (MyRTC_Time[2] >= 32)
					{
						MyRTC_Time[2] = 1;
					}
					break;
				}
				case 4:  // 小时  // 小时
				{
					MyRTC_Time[3]++;
					if (MyRTC_Time[3] >= 24)
					{
						MyRTC_Time[3] = 0;
					}
					break;
				}
				case 5:  // 分钟  // 分钟
				{
					MyRTC_Time[4]++;
					if (MyRTC_Time[4] >= 60)
					{
						MyRTC_Time[4] = 0;
					}
					break;
				}
				case 6:  // 秒  // 秒
				{
					MyRTC_Time[5]++;
					if (MyRTC_Time[5] >= 60)
					{
						MyRTC_Time[5] = 0;
					}
					break;
				}
				}
			}
		}
		if (Key_Check(KEY_3, KEY_SINGLE))
		{
			SwitchDateFlag = !SwitchDateFlag;
		}
		else if (Key_Check(KEY_3, KEY_LONG))
		{
			LCD_Clear(WHITE);
			LCD_ShowChinese(0, 0, "设置", LCD_16X16, BLACK, WHITE, 1);
			MyRTC_SetTime();
			return 0;
		}
		if (SwitchDateFlag == 0)
		{
			LCD_ShowChinese(0, 120, "切换", LCD_16X16, BLACK, WHITE, 1);
		}
		else
		{
			LCD_ShowChinese(0, 120, "调整", LCD_16X16, BLACK, WHITE, 1);
		}
		switch (DateFlag)
		{
		case 1:																	  // 设置年份
			LCD_ShowNum(40, 40, MyRTC_Time[0], LCD_8X16, WHITE, BLACK, 0, 0, 4);  // 年
			LCD_ShowNum(80, 40, MyRTC_Time[1], LCD_8X16, BLACK, WHITE, 0, 1, 2);  // 月
			LCD_ShowNum(104, 40, MyRTC_Time[2], LCD_8X16, BLACK, WHITE, 0, 1, 2); // 日
			LCD_ShowNum(48, 60, MyRTC_Time[3], LCD_8X16, BLACK, WHITE, 0, 1, 2);  // 时
			LCD_ShowNum(80, 60, MyRTC_Time[4], LCD_8X16, BLACK, WHITE, 0, 1, 2);  // 分
			LCD_ShowNum(104, 60, MyRTC_Time[5], LCD_8X16, BLACK, WHITE, 0, 1, 2); // 秒
			break;

		case 2:																	  // 设置月份
			LCD_ShowNum(40, 40, MyRTC_Time[0], LCD_8X16, BLACK, WHITE, 0, 0, 4);  // 年
			LCD_ShowNum(80, 40, MyRTC_Time[1], LCD_8X16, WHITE, BLACK, 0, 1, 2);  // 月
			LCD_ShowNum(104, 40, MyRTC_Time[2], LCD_8X16, BLACK, WHITE, 0, 1, 2); // 日
			LCD_ShowNum(48, 60, MyRTC_Time[3], LCD_8X16, BLACK, WHITE, 0, 1, 2);  // 时
			LCD_ShowNum(80, 60, MyRTC_Time[4], LCD_8X16, BLACK, WHITE, 0, 1, 2);  // 分
			LCD_ShowNum(104, 60, MyRTC_Time[5], LCD_8X16, BLACK, WHITE, 0, 1, 2); // 秒
			break;

		case 3:																	  // 设置日期
			LCD_ShowNum(40, 40, MyRTC_Time[0], LCD_8X16, BLACK, WHITE, 0, 0, 4);  // 年
			LCD_ShowNum(80, 40, MyRTC_Time[1], LCD_8X16, BLACK, WHITE, 0, 1, 2);  // 月
			LCD_ShowNum(104, 40, MyRTC_Time[2], LCD_8X16, WHITE, BLACK, 0, 1, 2); // 日
			LCD_ShowNum(48, 60, MyRTC_Time[3], LCD_8X16, BLACK, WHITE, 0, 1, 2);  // 时
			LCD_ShowNum(80, 60, MyRTC_Time[4], LCD_8X16, BLACK, WHITE, 0, 1, 2);  // 分
			LCD_ShowNum(104, 60, MyRTC_Time[5], LCD_8X16, BLACK, WHITE, 0, 1, 2); // 秒
			break;

		case 4:																	  // 设置小时
			LCD_ShowNum(40, 40, MyRTC_Time[0], LCD_8X16, BLACK, WHITE, 0, 0, 4);  // 年
			LCD_ShowNum(80, 40, MyRTC_Time[1], LCD_8X16, BLACK, WHITE, 0, 1, 2);  // 月
			LCD_ShowNum(104, 40, MyRTC_Time[2], LCD_8X16, BLACK, WHITE, 0, 1, 2); // 日
			LCD_ShowNum(48, 60, MyRTC_Time[3], LCD_8X16, WHITE, BLACK, 0, 1, 2);  // 时
			LCD_ShowNum(80, 60, MyRTC_Time[4], LCD_8X16, BLACK, WHITE, 0, 1, 2);  // 分
			LCD_ShowNum(104, 60, MyRTC_Time[5], LCD_8X16, BLACK, WHITE, 0, 1, 2); // 秒
			break;

		case 5:																	  // 设置分钟
			LCD_ShowNum(40, 40, MyRTC_Time[0], LCD_8X16, BLACK, WHITE, 0, 0, 4);  // 年
			LCD_ShowNum(80, 40, MyRTC_Time[1], LCD_8X16, BLACK, WHITE, 0, 1, 2);  // 月
			LCD_ShowNum(104, 40, MyRTC_Time[2], LCD_8X16, BLACK, WHITE, 0, 1, 2); // 日
			LCD_ShowNum(48, 60, MyRTC_Time[3], LCD_8X16, BLACK, WHITE, 0, 1, 2);  // 时
			LCD_ShowNum(80, 60, MyRTC_Time[4], LCD_8X16, WHITE, BLACK, 0, 1, 2);  // 分
			LCD_ShowNum(104, 60, MyRTC_Time[5], LCD_8X16, BLACK, WHITE, 0, 1, 2); // 秒
			break;

		case 6:																	  // 设置秒
			LCD_ShowNum(40, 40, MyRTC_Time[0], LCD_8X16, BLACK, WHITE, 0, 0, 4);  // 年
			LCD_ShowNum(80, 40, MyRTC_Time[1], LCD_8X16, BLACK, WHITE, 0, 1, 2);  // 月
			LCD_ShowNum(104, 40, MyRTC_Time[2], LCD_8X16, BLACK, WHITE, 0, 1, 2); // 日
			LCD_ShowNum(48, 60, MyRTC_Time[3], LCD_8X16, BLACK, WHITE, 0, 1, 2);  // 时
			LCD_ShowNum(80, 60, MyRTC_Time[4], LCD_8X16, BLACK, WHITE, 0, 1, 2);  // 分
			LCD_ShowNum(104, 60, MyRTC_Time[5], LCD_8X16, WHITE, BLACK, 0, 1, 2); // 秒
			break;
		}
	}
}

int Menu3_SetLight(void)
{
	static uint16_t LastLightValue = 0xFFFF;
	LCD_ShowChinese(0, 0, "设置", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 40, "日期时间", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 60, "光照阈值", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 80, "温度阈值", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 100, "单次步长", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowNum(72, 100, StepLength, LCD_8X16, BLACK, WHITE, 0, 0, 0);
	LCD_ShowNum(72, 60, LightValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
	LCD_ShowNum(72, 80, TempValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
	LCD_ShowString(64, 60, ">", LCD_8X16, WHITE, BLACK, 0);
	LastLightValue = LightValue;

	while (1)
	{
		if (Key_Check(KEY_1, KEY_UP) || Key_Check(KEY_1, KEY_REPEAT))
		{
			LightValue = LightValue - StepLength;
			if (LightValue <= 0)
			{
				LightValue = 999;
			}
		}
		if (Key_Check(KEY_2, KEY_UP) || Key_Check(KEY_2, KEY_REPEAT))
		{
			LightValue = LightValue + StepLength;
			if (LightValue >= 999)
			{
				LightValue = 0;
			}
		}
		if (Key_Check(KEY_3, KEY_SINGLE))
		{
			LCD_Clear(WHITE);
			LastLightValue = 0xFFFF;
			return 0;
		}

		if (LightValue != LastLightValue)
		{
			LCD_DrawRectangle_Fill(72, 60, 110, 75, WHITE);
			LCD_ShowNum(72, 60, LightValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
			LastLightValue = LightValue;
		}
	}
}

int Menu3_SetTemp(void)
{
	static int LastTempValue = 0xFF;
	LCD_ShowChinese(0, 0, "设置", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 40, "日期时间", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 60, "光照阈值", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 80, "温度阈值", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 100, "单次步长", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowNum(72, 100, StepLength, LCD_8X16, BLACK, WHITE, 0, 0, 0);
	LCD_ShowNum(72, 60, LightValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
	LCD_ShowNum(72, 80, TempValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
	LCD_ShowString(64, 80, ">", LCD_8X16, WHITE, BLACK, 0);
	LastTempValue = TempValue;

	while (1)
	{
		if (Key_Check(KEY_1, KEY_UP) || Key_Check(KEY_1, KEY_REPEAT))
		{
			TempValue = TempValue - StepLength;
			if (TempValue <= -40)
			{
				TempValue = 115;
			}
		}
		if (Key_Check(KEY_2, KEY_UP) || Key_Check(KEY_2, KEY_REPEAT))
		{
			TempValue = TempValue + StepLength;
			if (TempValue >= 115)
			{
				TempValue = -40;
			}
		}
		if (Key_Check(KEY_3, KEY_SINGLE))
		{
			LCD_Clear(WHITE);
			LastTempValue = 0xFF;
			return 0;
		}

		if (TempValue != LastTempValue)
		{
			LCD_DrawRectangle_Fill(72, 80, 110, 95, WHITE);
			LCD_ShowNum(72, 80, TempValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
			LastTempValue = TempValue;
		}
	}
}

int Menu3_SetLength(void)
{
	static uint8_t LastStepLength = 0xFF;
	LCD_ShowChinese(0, 0, "设置", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 40, "日期时间", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 60, "光照阈值", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 80, "温度阈值", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 100, "单次步长", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowNum(72, 100, StepLength, LCD_8X16, BLACK, WHITE, 0, 0, 0);
	LCD_ShowNum(72, 60, LightValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
	LCD_ShowNum(72, 80, TempValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
	LCD_ShowString(64, 100, ">", LCD_8X16, WHITE, BLACK, 0);
	LastStepLength = StepLength;

	while (1)
	{
		if (Key_Check(KEY_1, KEY_UP) || Key_Check(KEY_1, KEY_REPEAT))
		{
			StepLength--;
			if (StepLength == 0)
			{
				StepLength = 100;
			}
		}
		if (Key_Check(KEY_2, KEY_UP) || Key_Check(KEY_2, KEY_REPEAT))
		{
			StepLength++;
			if (StepLength == 101)
			{
				StepLength = 0;
			}
		}
		if (Key_Check(KEY_3, KEY_SINGLE))
		{
			LCD_Clear(WHITE);
			LastStepLength = 0xFF;
			return 0;
		}

		if (StepLength != LastStepLength)
		{
			LCD_DrawRectangle_Fill(72, 100, 110, 115, WHITE);
			LCD_ShowNum(72, 100, StepLength, LCD_8X16, BLACK, WHITE, 0, 0, 0);
			LastStepLength = StepLength;
		}
	}
}

int Menu3_AutoSaveSet(void)
{
	uint8_t AutoSaveFlag = 1;
	uint8_t Menu3_ToggleSaveFlag = 0;
	LCD_ShowChinese(0, 0, "历史数据", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 40, "自动存储", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 60, "存储间隔", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowString(40, 100, ":", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowString(80, 100, ":", LCD_8X16, BLACK, WHITE, 0);
	SaveInterval_Time[0] = SaveInterval / 3600;
	SaveInterval_Time[1] = (SaveInterval % 3600) / 60;
	SaveInterval_Time[2] = (SaveInterval % 3600) % 60;
	LCD_ShowNum(16, 100, SaveInterval_Time[0], LCD_8X16, BLACK, WHITE, 0, 1, 2);
	LCD_ShowNum(56, 100, SaveInterval_Time[1], LCD_8X16, BLACK, WHITE, 0, 1, 2);
	LCD_ShowNum(96, 100, SaveInterval_Time[2], LCD_8X16, BLACK, WHITE, 0, 1, 2);
	if (ToggleSaveFlag == 0)
	{
		LCD_ShowChinese(80, 40, "关", LCD_16X16, BLACK, WHITE, 0);
	}
	else
	{
		LCD_ShowChinese(80, 40, "开", LCD_16X16, BLACK, WHITE, 0);
	}
	while (1)
	{
		if (Key_Check(KEY_1, KEY_UP) || Key_Check(KEY_1, KEY_REPEAT))
		{
			AutoSaveFlag--;
			if (AutoSaveFlag == 0)
			{
				AutoSaveFlag = 3;
			}
		}
		if (Key_Check(KEY_2, KEY_UP) || Key_Check(KEY_2, KEY_REPEAT))
		{
			AutoSaveFlag++;
			if (AutoSaveFlag == 4)
			{
				AutoSaveFlag = 1;
			}
		}
		if (Key_Check(KEY_3, KEY_SINGLE))
		{
			LCD_Clear(WHITE);
			Menu3_ToggleSaveFlag = AutoSaveFlag;
		}
		if (Menu3_ToggleSaveFlag == 1)
		{
			return 0;
		}
		if (Menu3_ToggleSaveFlag == 2)
		{
			Menu3_ToggleSaveFlag = Menu4_ToggleSave();
		}
		if (Menu3_ToggleSaveFlag == 3)
		{
			Menu3_ToggleSaveFlag = Menu4_SaveInterval();
		}
		switch (AutoSaveFlag)
		{
		case 1:
		{
			LCD_ShowString(0, 20, "<---", LCD_8X16, WHITE, BLACK, 0);
			LCD_ShowChinese(0, 40, "自动存储", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 60, "存储间隔", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 0, "历史数据", LCD_16X16, BLACK, WHITE, 1);
			LCD_ShowNum(16, 100, SaveInterval_Time[0], LCD_8X16, BLACK, WHITE, 0, 1, 2);
			LCD_ShowNum(56, 100, SaveInterval_Time[1], LCD_8X16, BLACK, WHITE, 0, 1, 2);
			LCD_ShowNum(96, 100, SaveInterval_Time[2], LCD_8X16, BLACK, WHITE, 0, 1, 2);
			LCD_ShowString(40, 100, ":", LCD_8X16, BLACK, WHITE, 0);
			LCD_ShowString(80, 100, ":", LCD_8X16, BLACK, WHITE, 0);
			if (ToggleSaveFlag == 0)
			{
				LCD_ShowChinese(80, 40, "关", LCD_16X16, BLACK, WHITE, 0);
			}
			else
			{
				LCD_ShowChinese(80, 40, "开", LCD_16X16, BLACK, WHITE, 0);
			}
			break;
		}
		case 2:
		{
			LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 40, "自动存储", LCD_16X16, WHITE, BLACK, 0);
			LCD_ShowChinese(0, 60, "存储间隔", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 0, "历史数据", LCD_16X16, BLACK, WHITE, 1);
			LCD_ShowNum(16, 100, SaveInterval_Time[0], LCD_8X16, BLACK, WHITE, 0, 1, 2);
			LCD_ShowNum(56, 100, SaveInterval_Time[1], LCD_8X16, BLACK, WHITE, 0, 1, 2);
			LCD_ShowNum(96, 100, SaveInterval_Time[2], LCD_8X16, BLACK, WHITE, 0, 1, 2);
			LCD_ShowString(40, 100, ":", LCD_8X16, BLACK, WHITE, 0);
			LCD_ShowString(80, 100, ":", LCD_8X16, BLACK, WHITE, 0);
			if (ToggleSaveFlag == 0)
			{
				LCD_ShowChinese(80, 40, "关", LCD_16X16, BLACK, WHITE, 0);
			}
			else
			{
				LCD_ShowChinese(80, 40, "开", LCD_16X16, BLACK, WHITE, 0);
			}
			break;
		}
		case 3:
		{
			LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 40, "自动存储", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 60, "存储间隔", LCD_16X16, WHITE, BLACK, 0);
			LCD_ShowChinese(0, 0, "历史数据", LCD_16X16, BLACK, WHITE, 1);
			LCD_ShowNum(16, 100, SaveInterval_Time[0], LCD_8X16, BLACK, WHITE, 0, 1, 2);
			LCD_ShowNum(56, 100, SaveInterval_Time[1], LCD_8X16, BLACK, WHITE, 0, 1, 2);
			LCD_ShowNum(96, 100, SaveInterval_Time[2], LCD_8X16, BLACK, WHITE, 0, 1, 2);
			LCD_ShowString(40, 100, ":", LCD_8X16, BLACK, WHITE, 0);
			LCD_ShowString(80, 100, ":", LCD_8X16, BLACK, WHITE, 0);
			if (ToggleSaveFlag == 0)
			{
				LCD_ShowChinese(80, 40, "关", LCD_16X16, BLACK, WHITE, 0);
			}
			else
			{
				LCD_ShowChinese(80, 40, "开", LCD_16X16, BLACK, WHITE, 0);
			}
			break;
		}
		}
		if (SaveFlag == 1 && ToggleSaveFlag == 1)
		{
			MyRTC_ReadTime();
			CurrentTime.Year = MyRTC_Time[0];
			CurrentTime.Month = MyRTC_Time[1];
			CurrentTime.Day = MyRTC_Time[2];
			CurrentTime.Hour = MyRTC_Time[3];
			CurrentTime.Minute = MyRTC_Time[4];
			CurrentTime.Second = MyRTC_Time[5];
			Temp = DS18B20_Get_Temp();
			Light = LDR_LuxData();
			SaveFlag = 0;
			uint8_t saveResult = DataStorage_Save(Temp, Light, &CurrentTime);
			if (saveResult == 1)
			{
				ToggleSaveFlag = 0;
			}
		}

		// 持续显示存储状态
		if (ToggleSaveFlag == 1)
		{
			LCD_ShowChinese(80, 140, "存储中", LCD_16X16, BLUE, WHITE, 1);
		}
		else
		{
			NowTotalRecords = DataStorage_GetCount();
			if (NowTotalRecords >= MaxRecordCount)
			{
				LCD_ShowChinese(80, 140, " 已满 ", LCD_16X16, RED, WHITE, 1);
			}
		}
	}
}

int Menu3_ReadRecord(void)
{
	uint8_t ReadRecordFlag = 1;
	uint8_t Menu3_ReadRecordFlag = 0;
	uint8_t RefreshFlag = 1;
	LCD_ShowChinese(0, 0, "历史数据", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowString(0, 20, "<---", LCD_8X16, WHITE, BLACK, 0);
	LCD_ShowChinese(0, 80, "光照(lux):", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 40, "日期:", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 60, "时间:", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 100, "温度(C):", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowString(0, 120, "ID:", LCD_8X16, BLACK, WHITE, 0);

	while (1)
	{
		NowTotalRecords = DataStorage_GetCount();
		if (Key_Check(KEY_1, KEY_UP) || Key_Check(KEY_1, KEY_REPEAT))
		{
			if (ShowRecordID > 0)
			{
				ShowRecordID--;
			}
			RefreshFlag = 1;
		}
		if (Key_Check(KEY_2, KEY_UP) || Key_Check(KEY_2, KEY_REPEAT))
		{
			if (ShowRecordID < NowTotalRecords - 1)
			{
				ShowRecordID++;
			}
			RefreshFlag = 1;
		}
		if (Key_Check(KEY_3, KEY_SINGLE))
		{
			LCD_Clear(WHITE);
			Menu3_ReadRecordFlag = ReadRecordFlag;
		}
		if (Menu3_ReadRecordFlag == 1)
		{
			return 0;
		}
		if (RefreshFlag)
		{
			LCD_ShowRecord(ShowRecordID);
			RefreshFlag = 0;
		}
	}
}

int Menu3_ChipErase(void)
{
	LCD_Clear(WHITE);
	uint8_t Menu3_EraseFlag = 0;
	uint8_t EraseFlag = 2;
	LCD_ShowChinese(0, 40, "确认擦除?", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowChinese(20, 72, "是", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(88, 72, "否", LCD_16X16, BLACK, WHITE, 0);
	while (1)
	{
		if (Key_Check(KEY_1, KEY_UP) || Key_Check(KEY_1, KEY_REPEAT))
		{
			EraseFlag--;
			if (EraseFlag == 0)
			{
				EraseFlag = 2;
			}
		}
		if (Key_Check(KEY_2, KEY_UP) || Key_Check(KEY_2, KEY_REPEAT))
		{
			EraseFlag++;
			if (EraseFlag == 3)
			{
				EraseFlag = 1;
			}
		}
		if (Key_Check(KEY_3, KEY_SINGLE))
		{
			LCD_Clear(WHITE);
			Menu3_EraseFlag = EraseFlag;
		}
		if (Menu3_EraseFlag == 1)
		{
			Menu4_Erase();
			return 0;
		}
		if (Menu3_EraseFlag == 2)
		{
			return 0;
		}
		switch (EraseFlag)
		{
		case 1:
		{
			LCD_ShowChinese(20, 72, "是", LCD_16X16, WHITE, BLACK, 0);
			LCD_ShowChinese(88, 72, "否", LCD_16X16, BLACK, WHITE, 0);
			break;
		}
		case 2:
		{
			LCD_ShowChinese(20, 72, "是", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(88, 72, "否", LCD_16X16, WHITE, BLACK, 0);
			break;
		}
		}
	}
}

int Menu3_SetMaxRecord(void)
{
	uint8_t tempMax = MaxRecordCount;
	while (1)
	{
		if (ToggleSaveFlag == 0)
		{
			if (Key_Check(KEY_1, KEY_UP) || Key_Check(KEY_1, KEY_REPEAT))
			{
				tempMax--;
				if (tempMax == 0)
				{
					tempMax = 250;
				}
			}
			if (Key_Check(KEY_2, KEY_UP) || Key_Check(KEY_2, KEY_REPEAT))
			{
				tempMax++;
				if (tempMax == 251)
				{
					tempMax = 1;
				}
			}
			if (Key_Check(KEY_3, KEY_SINGLE))
			{
				uint8_t saveResult = DataStorage_SetMaxRecord(tempMax);
				LCD_Clear(WHITE);
				return 0;
			}
		}
		LCD_ShowChinese(0, 0, "历史数据", LCD_16X16, BLACK, WHITE, 1);
		LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
		LCD_ShowChinese(0, 40, "自动存储", LCD_16X16, BLACK, WHITE, 0);
		LCD_ShowChinese(0, 60, "读取记录", LCD_16X16, BLACK, WHITE, 0);
		LCD_ShowChinese(0, 80, "擦除数据", LCD_16X16, BLACK, WHITE, 0);
		LCD_ShowChinese(0, 100, "最大记录数", LCD_16X16, BLACK, WHITE, 0);
		LCD_ShowChinese(0, 120, "当前记录数", LCD_16X16, BLACK, WHITE, 0);
		LCD_ShowNum(96, 100, tempMax, LCD_8X16, BLACK, WHITE, 0, 1, 3);
		LCD_ShowNum(96, 120, NowTotalRecords, LCD_8X16, BLACK, WHITE, 0, 1, 3);
		LCD_ShowString(80, 100, ">", LCD_8X16, WHITE, BLACK, 0);
	}
}

int Menu4_Erase(void)
{
	LCD_Clear(WHITE);
	LCD_ShowChinese(0, 40, "擦除中...", LCD_16X16, RED, WHITE, 1);
	W25Q64_SectorErase(STORAGE_START_ADDR);
	W25Q64_WaitBusy();			// 确保擦除完成
	DataStorage_ResetCount();	// 重置内部计数
	ShowRecordID = 0;			// 重置显示ID
	NowTotalRecords = 0;		// 重置总记录数
	Delay_s(3);
	LCD_Clear(WHITE);
	return 0;
}

int Menu4_Start_StopWatch(void)
{
	StopWatchStartFlag = 1;
	return 0;
}

int Menu4_Pause_StopWatch(void)
{
	StopWatchStartFlag = 0;
	return 0;
}

int Menu4_Clear_StopWatch(void)
{
	StopWatchStartFlag = 0;
	hour = minute = second = 0;
	return 0;
}

int Menu3_StopWatch(void)
{
	uint8_t StopWatchFlag = 1;
	uint8_t StopWatchTemp = 0;
	static uint8_t LastHour = 0xFF, LastMinute = 0xFF, LastSecond = 0xFF;

	LCD_ShowChinese(0, 0, "秒表", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowChinese(8, 100, "开始", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(48, 100, "暂停", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(88, 100, "清除", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowString(48, 60, ":", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowString(72, 60, ":", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowNum(32, 60, hour, LCD_8X16, BLACK, WHITE, 0, 1, 2);
	LCD_ShowNum(56, 60, minute, LCD_8X16, BLACK, WHITE, 0, 1, 2);
	LCD_ShowNum(80, 60, second, LCD_8X16, BLACK, WHITE, 0, 1, 2);
	LastHour = hour;
	LastMinute = minute;
	LastSecond = second;

	while (1)
	{
		if (Key_Check(KEY_1, KEY_UP) || Key_Check(KEY_1, KEY_REPEAT))
		{
			StopWatchFlag--;
			if (StopWatchFlag == 0)
			{
				StopWatchFlag = 4;
			}
		}
		if (Key_Check(KEY_2, KEY_UP) || Key_Check(KEY_2, KEY_REPEAT))
		{
			StopWatchFlag++;
			if (StopWatchFlag == 5)
			{
				StopWatchFlag = 1;
			}
		}
		if (Key_Check(KEY_3, KEY_SINGLE))
		{
			StopWatchTemp = StopWatchFlag;
		}
		if (StopWatchTemp == 1)
		{
			LastHour = LastMinute = LastSecond = 0xFF;
			StopWatchStartFlag = 0;
			LCD_Clear(WHITE);
			return 0;
		}
		if (StopWatchTemp == 2)
		{
			Menu4_Start_StopWatch();
			StopWatchTemp = 0;
		}
		if (StopWatchTemp == 3)
		{
			Menu4_Pause_StopWatch();
			StopWatchTemp = 0;
		}
		if (StopWatchTemp == 4)
		{
			Menu4_Clear_StopWatch();
			StopWatchTemp = 0;
		}

		// 只在时间改变时更新显示
		if (hour != LastHour)
		{
			LCD_DrawRectangle_Fill(32, 60, 46, 75, WHITE);
			LCD_ShowNum(32, 60, hour, LCD_8X16, BLACK, WHITE, 0, 1, 2);
			LastHour = hour;
		}
		if (minute != LastMinute)
		{
			LCD_DrawRectangle_Fill(56, 60, 70, 75, WHITE);
			LCD_ShowNum(56, 60, minute, LCD_8X16, BLACK, WHITE, 0, 1, 2);
			LastMinute = minute;
		}
		if (second != LastSecond)
		{
			LCD_DrawRectangle_Fill(80, 60, 94, 75, WHITE);
			LCD_ShowNum(80, 60, second, LCD_8X16, BLACK, WHITE, 0, 1, 2);
			LastSecond = second;
		}

		switch (StopWatchFlag)
		{
		case 1:
		{
			LCD_ShowChinese(0, 0, "秒表", LCD_16X16, BLACK, WHITE, 1);
			LCD_ShowString(0, 20, "<---", LCD_8X16, WHITE, BLACK, 0);
			LCD_ShowChinese(8, 100, "开始", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(48, 100, "暂停", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(88, 100, "清除", LCD_16X16, BLACK, WHITE, 0);
			break;
		}
		case 2:
		{
			LCD_ShowChinese(0, 0, "秒表", LCD_16X16, BLACK, WHITE, 1);
			LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
			LCD_ShowChinese(8, 100, "开始", LCD_16X16, WHITE, BLACK, 0);
			LCD_ShowChinese(48, 100, "暂停", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(88, 100, "清除", LCD_16X16, BLACK, WHITE, 0);
			break;
		}
		case 3:
		{
			LCD_ShowChinese(0, 0, "秒表", LCD_16X16, BLACK, WHITE, 1);
			LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
			LCD_ShowChinese(8, 100, "开始", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(48, 100, "暂停", LCD_16X16, WHITE, BLACK, 0);
			LCD_ShowChinese(88, 100, "清除", LCD_16X16, BLACK, WHITE, 0);
			break;
		}
		case 4:
		{
			LCD_ShowChinese(0, 0, "秒表", LCD_16X16, BLACK, WHITE, 1);
			LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
			LCD_ShowChinese(8, 100, "开始", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(48, 100, "暂停", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(88, 100, "清除", LCD_16X16, WHITE, BLACK, 0);
			break;
		}
		}
	}
}

int Menu4_ToggleSave(void)
{
	LCD_ShowChinese(0, 0, "历史数据", LCD_16X16, BLACK, WHITE, 1);
	SaveInterval_Time[0] = SaveInterval / 3600;
	SaveInterval_Time[1] = (SaveInterval % 3600) / 60;
	SaveInterval_Time[2] = (SaveInterval % 3600) % 60;
	LCD_ShowString(40, 100, ":", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowString(80, 100, ":", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowNum(16, 100, SaveInterval_Time[0], LCD_8X16, BLACK, WHITE, 0, 1, 2);
	LCD_ShowNum(56, 100, SaveInterval_Time[1], LCD_8X16, BLACK, WHITE, 0, 1, 2);
	LCD_ShowNum(96, 100, SaveInterval_Time[2], LCD_8X16, BLACK, WHITE, 0, 1, 2);
	while (1)
	{
		if (Key_Check(KEY_1, KEY_UP) || Key_Check(KEY_1, KEY_REPEAT))
		{
			ToggleSaveFlag = !ToggleSaveFlag;
		}
		if (Key_Check(KEY_2, KEY_UP) || Key_Check(KEY_2, KEY_REPEAT))
		{
			ToggleSaveFlag = !ToggleSaveFlag;
		}
		if (Key_Check(KEY_3, KEY_SINGLE))
		{
			LCD_Clear(WHITE);
			return 0;
		}
		LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
		LCD_ShowChinese(0, 40, "自动存储", LCD_16X16, BLACK, WHITE, 0);
		LCD_ShowChinese(0, 60, "存储间隔", LCD_16X16, BLACK, WHITE, 0);
		LCD_ShowString(72, 40, ">", LCD_8X16, WHITE, BLACK, 0);
		if (ToggleSaveFlag == 0)
		{
			LCD_ShowChinese(80, 40, "关", LCD_16X16, BLACK, WHITE, 0);
		}
		else
		{
			LCD_ShowChinese(80, 40, "开", LCD_16X16, BLACK, WHITE, 0);
		}
	}
}

int Menu4_SaveInterval(void)
{
	uint8_t SaveIntervalFlag = 1;
	uint8_t SwitchFlag = 0;
	LCD_ShowChinese(0, 0, "历史数据", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 40, "自动存储", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 60, "存储间隔", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowString(40, 100, ":", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowString(80, 100, ":", LCD_8X16, BLACK, WHITE, 0);
	SaveInterval_Time[0] = SaveInterval / 3600;
	SaveInterval_Time[1] = (SaveInterval % 3600) / 60;
	SaveInterval_Time[2] = (SaveInterval % 3600) % 60;
	while (1)
	{
		if (SwitchFlag == 0)
		{
			if (Key_Check(KEY_1, KEY_UP) || Key_Check(KEY_1, KEY_REPEAT)) // KEY_1：上一项（小时→秒循环）
			{
				SaveIntervalFlag--;
				if (SaveIntervalFlag == 0)
				{
					SaveIntervalFlag = 3;
				}
			}
			else if (Key_Check(KEY_2, KEY_UP)|| Key_Check(KEY_2, KEY_REPEAT)) // KEY_2：下一项（秒→小时循环）
			{
				SaveIntervalFlag++;
				if (SaveIntervalFlag == 4)
				{
					SaveIntervalFlag = 1;
				}
			}
		}
		else if (SwitchFlag == 1)
		{
			if (Key_Check(KEY_1, KEY_UP) || Key_Check(KEY_1, KEY_REPEAT)) // KEY_1：减少
			{
				switch (SaveIntervalFlag)
				{
				case 1:
				{
					if (SaveInterval_Time[0] == 0)
					{
						SaveInterval_Time[0] = 23;
					}
					else
					{
						SaveInterval_Time[0]--;
					}
					break;
				}
				case 2:
				{
					if (SaveInterval_Time[1] == 0)
					{
						SaveInterval_Time[1] = 59;
					}
					else
					{
						SaveInterval_Time[1]--;
					}
					break;
				}
				case 3:
				{
					if (SaveInterval_Time[2] == 0)
					{
						SaveInterval_Time[2] = 59;
					}
					else
					{
						SaveInterval_Time[2]--;
					}
					break;
				}
				}
				SaveInterval = SaveInterval_Time[0] * 3600 + SaveInterval_Time[1] * 60 + SaveInterval_Time[2];
				// 防止间隔为0，最小1秒
				if (SaveInterval == 0)
				{
					SaveInterval = 1;
					SaveInterval_Time[2] = 1;
				}
			}
			else if (Key_Check(KEY_2, KEY_UP)|| Key_Check(KEY_2, KEY_REPEAT)) // KEY_2：增加
			{
				switch (SaveIntervalFlag)
				{
				case 1:
				{
					SaveInterval_Time[0]++;
					if (SaveInterval_Time[0] == 24)
					{
						SaveInterval_Time[0] = 0;
					}
					break;
				}
				case 2:
				{
					SaveInterval_Time[1]++;
					if (SaveInterval_Time[1] == 60)
					{
						SaveInterval_Time[1] = 0;
					}
					break;
				}
				case 3:
				{
					SaveInterval_Time[2]++;
					if (SaveInterval_Time[2] == 60)
					{
						SaveInterval_Time[2] = 0;
					}
					break;
				}
				}
				SaveInterval = SaveInterval_Time[0] * 3600 + SaveInterval_Time[1] * 60 + SaveInterval_Time[2];
				// 防止间隔为0，最小1秒
				if (SaveInterval == 0)
				{
					SaveInterval = 1;
					SaveInterval_Time[2] = 1;
				}
			}
		}
		if (Key_Check(KEY_3, KEY_SINGLE))
		{
			SwitchFlag = !SwitchFlag;
		}
		else if (Key_Check(KEY_3, KEY_LONG))
		{
			LCD_Clear(WHITE);
			return 0;
		}
		
		// 显示当前模式
		if (SwitchFlag == 0)
		{
			LCD_ShowChinese(0, 120, "切换", LCD_16X16, BLACK, WHITE, 1);
		}
		else
		{
			LCD_ShowChinese(0, 120, "调整", LCD_16X16, BLACK, WHITE, 1);
		}
		
		switch (SaveIntervalFlag)
		{
		case 1:
		{
			LCD_ShowNum(16, 100, SaveInterval_Time[0], LCD_8X16, WHITE, BLACK, 0, 1, 2);
			LCD_ShowNum(56, 100, SaveInterval_Time[1], LCD_8X16, BLACK, WHITE, 0, 1, 2);
			LCD_ShowNum(96, 100, SaveInterval_Time[2], LCD_8X16, BLACK, WHITE, 0, 1, 2);
			break;
		}
		case 2:
		{
			LCD_ShowNum(16, 100, SaveInterval_Time[0], LCD_8X16, BLACK, WHITE, 0, 1, 2);
			LCD_ShowNum(56, 100, SaveInterval_Time[1], LCD_8X16, WHITE, BLACK, 0, 1, 2);
			LCD_ShowNum(96, 100, SaveInterval_Time[2], LCD_8X16, BLACK, WHITE, 0, 1, 2);
			break;
		}
		case 3:
		{
			LCD_ShowNum(16, 100, SaveInterval_Time[0], LCD_8X16, BLACK, WHITE, 0, 1, 2);
			LCD_ShowNum(56, 100, SaveInterval_Time[1], LCD_8X16, BLACK, WHITE, 0, 1, 2);
			LCD_ShowNum(96, 100, SaveInterval_Time[2], LCD_8X16, WHITE, BLACK, 0, 1, 2);
			break;
		}
		}
		if (ToggleSaveFlag == 0)
		{
			LCD_ShowChinese(80, 40, "关", LCD_16X16, BLACK, WHITE, 0);
		}
		else
		{
			LCD_ShowChinese(80, 40, "开", LCD_16X16, BLACK, WHITE, 0);
		}
	}
}

void LCD_ShowRecord(uint8_t RecordID)
{
	SensorData record;
	NowTotalRecords = DataStorage_GetCount();
	if (RecordID >= NowTotalRecords)
	{
		LCD_ShowChinese(32, 120, "无记录", LCD_16X16, BLACK, WHITE, 0);
		return;
	}
	DataStorage_Read(RecordID, &record);
	char IndexStr[20];
	float temp = record.Temp / 10.0f;
	sprintf((char *)IndexStr, "%03d/%03d", RecordID + 1, NowTotalRecords);
	LCD_ShowString(32, 120, IndexStr, LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowNum(40, 40, record.Year, LCD_8X16, BLACK, WHITE, 0, 1, 4);	 // 年，固定4位
	LCD_ShowNum(80, 40, record.Month, LCD_8X16, BLACK, WHITE, 0, 1, 2);	 // 月，固定2位
	LCD_ShowNum(104, 40, record.Day, LCD_8X16, BLACK, WHITE, 0, 1, 2);	 // 日，固定2位
	LCD_ShowNum(40, 60, record.Hour, LCD_8X16, BLACK, WHITE, 0, 1, 2);	 // 时，固定2位
	LCD_ShowNum(64, 60, record.Minute, LCD_8X16, BLACK, WHITE, 0, 1, 2); // 分，固定2位
	LCD_ShowNum(88, 60, record.Second, LCD_8X16, BLACK, WHITE, 0, 1, 2); // 秒，固定2位
	LCD_ShowNum(88, 80, record.Light, LCD_8X16, BLACK, WHITE, 0, 1, 3);  // 光照，固定3位
	LCD_ShowFloatNum(72, 100, temp, 1, LCD_8X16, BLACK, WHITE, 0, 6);    // 温度，固定6字符宽度

	LCD_ShowString(72, 40, "-", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowString(96, 40, "-", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowString(56, 60, ":", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowString(80, 60, ":", LCD_8X16, BLACK, WHITE, 0);
}

void StopWatch_Tick(void)
{
	static uint16_t StopWatchCount;
	StopWatchCount++;
	if (StopWatchCount >= 1000)
	{
		StopWatchCount = 0;
		if (StopWatchStartFlag == 1)
		{
			second++;
			if (second >= 60)
			{
				second = 0;
				minute++;
				if (minute >= 60)
				{
					minute = 0;
					hour++;
					if (hour > 99)
					{
						hour = 0;
					}
				}
			}
		}
	}
}

void TIM4_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) == SET)
	{
		Key_Tick();
		StopWatch_Tick();
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
	}
}

void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		TimerCount++;
		if (TimerCount >= SaveInterval)
		{
			TimerCount = 0;
			SaveFlag = 1;
		}
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}
