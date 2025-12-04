#include "Menu.h"
#include "IWDG.h"
#define Music_Count 5

uint8_t Music = 1;
extern int MyRTC_Time[];
int SaveInterval_Time[3];
DateTime_New CurrentTime;
int SaveInterval = TIME_SECONDS_PER_MINUTE;
uint8_t StopWatchStartFlag = 0;
uint8_t hour, minute, second;
uint8_t Flag = 1;
uint8_t StepLength = 5;
uint16_t LightValue = 400;
int TempValue = 35;
uint8_t ToggleSaveFlag = 0;
uint8_t NowTotalRecords;
uint8_t ShowRecordID = 0;
volatile float Temp = 0;
volatile uint16_t Light = 0;
volatile uint8_t SaveFlag = 0;
volatile int TimerCount = 0;
volatile uint32_t SystemTick = 0; // 系统滴答计数器(1ms)

// 辅助函数：原子读取并清零SaveFlag
static inline uint8_t SaveFlag_ReadAndClear(void)
{
	__disable_irq(); // 关闭全局中断
	uint8_t flag = SaveFlag;
	if (flag)
		SaveFlag = 0;
	__enable_irq(); // 恢复全局中断
	return flag;
}

// 菜单重绘辅助函数：历史数据菜单
static void Menu2_History_Redraw(void)
{
	LCD_Clear(WHITE);
	LCD_ShowChinese(0, 0, "历史数据", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 40, "读取记录", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 60, "数据图表", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 80, "擦除数据", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 100, "记录数", LCD_16X16, BLACK, WHITE, 0);
	NowTotalRecords = DataStorage_New_GetCount();
	LCD_ShowNum(64, 100, NowTotalRecords, LCD_8X16, BLACK, WHITE, 0, 1, 3);
	LCD_ShowString(88, 100, "/", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowNum(96, 100, DataStorage_New_GetMaxCount(), LCD_8X16, BLACK, WHITE, 0, 1, 3);
}

// 菜单重绘辅助函数：功能菜单
static void Menu2_Function_Redraw(void)
{
	LCD_Clear(WHITE);
	LCD_ShowChinese(0, 0, "功能", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 40, "音乐播放", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 60, "秒表", LCD_16X16, BLACK, WHITE, 0);
}

// 菜单重绘辅助函数：设置菜单
static void Menu2_Setting_Redraw(void)
{
	LCD_Clear(WHITE);
	LCD_ShowChinese(0, 0, "设置", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 40, "日期时间", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 60, "自动存储", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 80, "光照阈值", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 100, "温度阈值", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 120, "最大记录", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 140, "单次步长", LCD_16X16, BLACK, WHITE, 0);
}

// 存储状态枚举
typedef enum
{
	STORAGE_STATE_IDLE = 0,	  // 空闲/已停止
	STORAGE_STATE_SAVING = 1, // 存储中
	STORAGE_STATE_FULL = 2	  // 已满
} StorageState_t;

// 辅助函数：更新存储状态显示
// @param lastState: 上次状态指针（用于判断是否需要更新）
// @param yPos: 显示的Y坐标（默认140，Menu3_AutoSaveSet使用140）
static void Menu_UpdateStorageStatus(uint8_t *lastState, uint16_t yPos)
{
	uint8_t recordCount = DataStorage_New_GetCount();
	uint8_t maxCount = DataStorage_New_GetMaxCount();

	// 计算当前状态
	StorageState_t currentState;
	if (ToggleSaveFlag && recordCount < maxCount)
		currentState = STORAGE_STATE_SAVING; // 开启且未满
	else if (recordCount >= maxCount)
		currentState = STORAGE_STATE_FULL; // 已满
	else
		currentState = STORAGE_STATE_IDLE; // 关闭或其他

	// 仅在状态变化时更新显示
	if (currentState != *lastState)
	{
		switch (currentState)
		{
		case STORAGE_STATE_SAVING: // 存储中
			LCD_ShowChinese(80, yPos, "存储中", LCD_16X16, BLUE, WHITE, 1);
			break;
		case STORAGE_STATE_FULL: // 已满
			LCD_ShowChinese(80, yPos, " 已满 ", LCD_16X16, RED, WHITE, 1);
			break;
		default: // 空闲/停止
			LCD_ShowString(80, yPos, "       ", LCD_8X16, WHITE, WHITE, 0);
			break;
		}
		*lastState = currentState;
	}
}

// 辅助函数：更新设置菜单的数值显示
static void Menu2_Setting_UpdateValues(void)
{
	LCD_ShowNum(72, 80, LightValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
	LCD_ShowSignedNum(72, 100, TempValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
	LCD_ShowNum(72, 120, DataStorage_New_GetMaxCount(), LCD_8X16, BLACK, WHITE, 0, 0, 0);
	LCD_ShowNum(72, 140, StepLength, LCD_8X16, BLACK, WHITE, 0, 0, 0);
}

int Menu1(void) // 一级菜单
{
	uint8_t lastFlag = 0; // 记录上次选中的项
	LCD_Clear(WHITE);
	LCD_ShowChinese(0, 32, "状态信息", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowChinese(0, 56, "历史数据", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowChinese(0, 80, "功能", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowChinese(0, 104, "设置", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowChinese(0, 32, "状态信息", LCD_16X16, WHITE, BLACK, 1);
	lastFlag = 1;
	while (1)
	{
		IWDG_Feed();
		if (Encoder_Check(ENCODER_CW))
		{
			Flag--;
			if (Flag == 0)
				Flag = MENU_MAIN_ITEM_COUNT;
		}
		if (Encoder_Check(ENCODER_CCW))
		{
			Flag++;
			if (Flag == MENU_MAIN_ITEM_COUNT + 1)
				Flag = 1;
		}
		if (Encoder_Check(ENCODER_BTN_SINGLE))
		{
			LCD_Clear(WHITE);
			return Flag;
		}

		// 仅在选项变化时更新显示
		if (Flag != lastFlag)
		{
			// 1. 恢复旧选项为普通显示 (黑字白底)
			switch (lastFlag)
			{
			case 1:
				LCD_ShowChinese(0, 32, "状态信息", LCD_16X16, BLACK, WHITE, 1);
				break;
			case 2:
				LCD_ShowChinese(0, 56, "历史数据", LCD_16X16, BLACK, WHITE, 1);
				break;
			case 3:
				LCD_ShowChinese(0, 80, "功能", LCD_16X16, BLACK, WHITE, 1);
				break;
			case 4:
				LCD_ShowChinese(0, 104, "设置", LCD_16X16, BLACK, WHITE, 1);
				break;
			}

			// 2. 设置新选项为高亮显示 (白字黑底)
			switch (Flag)
			{
			case 1:
				LCD_ShowChinese(0, 32, "状态信息", LCD_16X16, WHITE, BLACK, 1);
				break;
			case 2:
				LCD_ShowChinese(0, 56, "历史数据", LCD_16X16, WHITE, BLACK, 1);
				break;
			case 3:
				LCD_ShowChinese(0, 80, "功能", LCD_16X16, WHITE, BLACK, 1);
				break;
			case 4:
				LCD_ShowChinese(0, 104, "设置", LCD_16X16, WHITE, BLACK, 1);
				break;
			}

			lastFlag = Flag;
		}
	}
}

int Menu2_Stats(void) // 二级菜单
{
	uint8_t StFlag = 1;
	uint8_t Menu2_StatsFlag = 0;
	uint8_t AlarmActiveFlag = 0;
	uint8_t AlarmCounter = 0;
	uint8_t LastAlarmState = 0;		 // 上次报警状态，用于检测状态变化
	uint8_t lastStorageState = 0xFF; // 存储状态缓存
	uint8_t LEDBlinkCounter = 0;	 // LED闪烁计数器
	const uint16_t MAX_ALARM_TICKS = ALARM_BEEP_DURATION;
	const uint8_t LED_BLINK_INTERVAL = 1; // LED闪烁间隔（主循环周期数）

	// 状态缓存变量
	uint8_t LastTime[6] = {0};
	uint16_t LastLight = 0xFFFF;	  // 初始值设为无效值以触发首次显示
	float LastTemp = -999.0;		  // 初始值设为无效值
	uint32_t LastLightUpdateTick = 0; // 光照刷新计时器

	LCD_Clear(WHITE);
	LCD_ShowChinese(0, 0, "状态信息", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowChinese(0, 20, "<---", LCD_16X16, WHITE, BLACK, 0);
	LCD_ShowChinese(0, 80, "光照(lux):", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 40, "日期:", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 60, "时间:", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 100, "温度(℃):", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowString(72, 40, "-", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowString(96, 40, "-", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowString(56, 60, ":", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowString(80, 60, ":", LCD_8X16, BLACK, WHITE, 0);

	// 首次强制刷新标志
	uint8_t firstRun = 1;

	while (1)
	{
		IWDG_Feed();
		if (Encoder_Check(ENCODER_BTN_SINGLE))
		{
			Buzzer_OFF();
			Menu2_StatsFlag = StFlag;
		}
		if (Menu2_StatsFlag == 1)
		{
			return 0;
		}

		// 读取传感器数据
		MyRTC_ReadTime();
		Light = LDR_LuxData();
		Temp = Temp_Average_Data();

		// 仅在时间变化时更新显示 (检查秒)
		if (firstRun || MyRTC_Time[5] != LastTime[5])
		{
			LCD_ShowNum(40, 40, MyRTC_Time[0], LCD_8X16, BLACK, WHITE, 0, 0, 4);  // 年
			LCD_ShowNum(80, 40, MyRTC_Time[1], LCD_8X16, BLACK, WHITE, 0, 1, 2);  // 月
			LCD_ShowNum(104, 40, MyRTC_Time[2], LCD_8X16, BLACK, WHITE, 0, 1, 2); // 日
			LCD_ShowNum(40, 60, MyRTC_Time[3], LCD_8X16, BLACK, WHITE, 0, 1, 2);  // 时
			LCD_ShowNum(64, 60, MyRTC_Time[4], LCD_8X16, BLACK, WHITE, 0, 1, 2);  // 分
			LCD_ShowNum(88, 60, MyRTC_Time[5], LCD_8X16, BLACK, WHITE, 0, 1, 2);  // 秒

			for (int i = 0; i < 6; i++)
				LastTime[i] = MyRTC_Time[i];
		}

		// 自动存储处理（原子读取标志）
		if (SaveFlag_ReadAndClear() && ToggleSaveFlag == 1)
		{
			CurrentTime.Year = MyRTC_Time[0];
			CurrentTime.Month = MyRTC_Time[1];
			CurrentTime.Day = MyRTC_Time[2];
			CurrentTime.Hour = MyRTC_Time[3];
			CurrentTime.Minute = MyRTC_Time[4];
			CurrentTime.Second = MyRTC_Time[5];
			// 读取实时传感器数据
			float tempData = DS18B20_Get_Temp();
			uint16_t lightData = LDR_LuxData();
			uint8_t saveResult = DataStorage_New_Save(tempData, lightData, &CurrentTime);
			if (saveResult == 1)
			{
				ToggleSaveFlag = 0; // 存储已满，停止自动存储
			}
		}

		// 持续显示存储状态（仅在状态变化时更新）
		uint8_t currentState = ToggleSaveFlag ? 1 : (DataStorage_New_GetCount() >= DataStorage_New_GetMaxCount() ? 2 : 0);
		if (currentState != lastStorageState)
		{
			if (currentState == 1)
			{
				LCD_ShowChinese(80, 140, "存储中", LCD_16X16, BLUE, WHITE, 1);
			}
			else if (currentState == 2)
			{
				LCD_ShowChinese(80, 140, " 已满 ", LCD_16X16, RED, WHITE, 1);
			}
			else
			{
				LCD_ShowString(80, 140, "       ", LCD_8X16, WHITE, WHITE, 0);
			}
			lastStorageState = currentState;
		}

		// 判断是否需要报警
		uint8_t CurrentAlarmState = (Temp >= TempValue || Light >= LightValue) ? 1 : 0;

		// 只在状态变化时更新中文标签，减少闪烁
		if (CurrentAlarmState != LastAlarmState)
		{
			if (CurrentAlarmState)
			{
				LCD_ShowChinese(0, 80, "光照(lux):", LCD_16X16, RED, WHITE, 0);
				LCD_ShowChinese(0, 100, "温度(℃):", LCD_16X16, RED, WHITE, 0);
			}
			else
			{
				LCD_ShowChinese(0, 80, "光照(lux):", LCD_16X16, BLACK, WHITE, 0);
				LCD_ShowChinese(0, 100, "温度(℃):", LCD_16X16, BLACK, WHITE, 0);
			}
			LastAlarmState = CurrentAlarmState;
			// 状态改变时强制刷新数值
			LastLight = 0xFFFF;
			LastTemp = -999.0;
		}

		// 显示数值（仅在数值变化或报警状态变化时更新）
		uint16_t displayColor = CurrentAlarmState ? RED : BLACK;

		// 光照显示：增加200ms刷新延迟限制
		if (firstRun || (SystemTick - LastLightUpdateTick >= 200))
		{
			if (Light != LastLight)
			{
				LCD_ShowNum(88, 80, Light, LCD_8X16, displayColor, WHITE, 0, 1, 3);
				LastLight = Light;
			}
			LastLightUpdateTick = SystemTick;
		}

		if (firstRun || Temp != LastTemp)
		{
			// 温度显示，FixedWidth=6可容纳"-99.9"格式
			LCD_ShowFloatNum(72, 100, Temp, 1, LCD_8X16, displayColor, WHITE, 0, 6);
			LastTemp = Temp;
		}

		firstRun = 0;

		// 报警逻辑处理
		if (CurrentAlarmState)
		{
			if (!AlarmActiveFlag)
			{
				Buzzer_ON();
				LED_ON(); // 报警开始时点亮LED
				AlarmActiveFlag = 1;
				AlarmCounter = 0;
				LEDBlinkCounter = 0;
			}
			else
			{
				AlarmCounter++;
				if (AlarmCounter >= MAX_ALARM_TICKS)
				{
					Buzzer_OFF();
					AlarmActiveFlag = 0;
				}

				// LED持续闪烁（不受蜂鸣器停止影响）
				LEDBlinkCounter++;
				if (LEDBlinkCounter >= LED_BLINK_INTERVAL)
				{
					LED_Turn(); // 翻转LED状态
					LEDBlinkCounter = 0;
				}
			}
		}
		else
		{
			if (AlarmActiveFlag)
			{
				Buzzer_OFF();
				LED_ON(); // 报警结束时关闭LED
				AlarmActiveFlag = 0;
				AlarmCounter = 0;
				LEDBlinkCounter = 0;
			}
		}
	}
}

int Menu2_Setting(void)
{
	uint8_t Menu3_Set = 0;
	uint8_t SetFlag = 1;
	uint8_t lastSetFlag = 0;
	LCD_Clear(WHITE);
	LCD_ShowString(0, 20, "<---", LCD_8X16, WHITE, BLACK, 0); // 初始高亮
	LCD_ShowChinese(0, 40, "日期时间", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 60, "自动存储", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 80, "光照阈值", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 100, "温度阈值", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 120, "最大记录", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 140, "单次步长", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 0, "设置", LCD_16X16, BLACK, WHITE, 1);

	LCD_ShowNum(72, 80, LightValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
	LCD_ShowSignedNum(72, 100, TempValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
	LCD_ShowNum(72, 120, DataStorage_New_GetMaxCount(), LCD_8X16, BLACK, WHITE, 0, 0, 0);
	LCD_ShowNum(72, 140, StepLength, LCD_8X16, BLACK, WHITE, 0, 0, 0);

	lastSetFlag = 1;
	while (1)
	{
		IWDG_Feed();
		// 自动存储处理（读取实时传感器数据，原子读取标志）
		if (SaveFlag_ReadAndClear() && ToggleSaveFlag == 1)
		{
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
			uint8_t saveResult = DataStorage_New_Save(tempData, lightData, &CurrentTime);
			if (saveResult == 1)
			{
				ToggleSaveFlag = 0; // 存储已满，停止自动存储
			}
		}

		if (Encoder_Check(ENCODER_CW))
		{
			SetFlag--;
			if (SetFlag == 0)
			{
				SetFlag = MENU_SETTING_ITEM_COUNT;
			}
		}
		if (Encoder_Check(ENCODER_CCW))
		{
			SetFlag++;
			if (SetFlag == MENU_SETTING_ITEM_COUNT + 1)
			{
				SetFlag = 1;
			}
		}

		// 只在菜单项变化时更新高亮
		if (SetFlag != lastSetFlag)
		{
			// 恢复旧项
			switch (lastSetFlag)
			{
			case 1:
				LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
				break;
			case 2:
				LCD_ShowChinese(0, 40, "日期时间", LCD_16X16, BLACK, WHITE, 0);
				break;
			case 3:
				LCD_ShowChinese(0, 60, "自动存储", LCD_16X16, BLACK, WHITE, 0);
				break;
			case 4:
				LCD_ShowChinese(0, 80, "光照阈值", LCD_16X16, BLACK, WHITE, 0);
				break;
			case 5:
				LCD_ShowChinese(0, 100, "温度阈值", LCD_16X16, BLACK, WHITE, 0);
				break;
			case 6:
				LCD_ShowChinese(0, 120, "最大记录", LCD_16X16, BLACK, WHITE, 0);
				break;
			case 7:
				LCD_ShowChinese(0, 140, "单次步长", LCD_16X16, BLACK, WHITE, 0);
				break;
			}
			// 高亮新项
			switch (SetFlag)
			{
			case 1:
				LCD_ShowString(0, 20, "<---", LCD_8X16, WHITE, BLACK, 0);
				break;
			case 2:
				LCD_ShowChinese(0, 40, "日期时间", LCD_16X16, WHITE, BLACK, 0);
				break;
			case 3:
				LCD_ShowChinese(0, 60, "自动存储", LCD_16X16, WHITE, BLACK, 0);
				break;
			case 4:
				LCD_ShowChinese(0, 80, "光照阈值", LCD_16X16, WHITE, BLACK, 0);
				break;
			case 5:
				LCD_ShowChinese(0, 100, "温度阈值", LCD_16X16, WHITE, BLACK, 0);
				break;
			case 6:
				LCD_ShowChinese(0, 120, "最大记录", LCD_16X16, WHITE, BLACK, 0);
				break;
			case 7:
				LCD_ShowChinese(0, 140, "单次步长", LCD_16X16, WHITE, BLACK, 0);
				break;
			}
			lastSetFlag = SetFlag;
		}

		if (Encoder_Check(ENCODER_BTN_SINGLE))
		{
			Menu3_Set = SetFlag;
		}
		if (Menu3_Set == 1)
		{
			return 0;
		}
		if (Menu3_Set == 2)
		{
			Menu3_Set = Menu3_SetDate();
			if (Menu3_Set == 0) // 返回后重绘界面
			{
				Menu2_Setting_Redraw();
				Menu2_Setting_UpdateValues();
				lastSetFlag = 0xFF; // 强制重绘高亮
			}
		}
		if (Menu3_Set == 3)
		{
			Menu3_Set = Menu3_AutoSaveSet();
			if (Menu3_Set == 0)
			{
				Menu2_Setting_Redraw();
				Menu2_Setting_UpdateValues();
				lastSetFlag = 0xFF;
			}
		}
		if (Menu3_Set == 4)
		{
			Menu3_Set = Menu3_SetLight();
			if (Menu3_Set == 0)
			{
				Menu2_Setting_Redraw();
				Menu2_Setting_UpdateValues();
				lastSetFlag = 0xFF;
			}
		}
		if (Menu3_Set == 5)
		{
			Menu3_Set = Menu3_SetTemp();
			if (Menu3_Set == 0)
			{
				Menu2_Setting_Redraw();
				Menu2_Setting_UpdateValues();
				lastSetFlag = 0xFF;
			}
		}
		if (Menu3_Set == 6)
		{
			Menu3_Set = Menu3_SetMaxRecord();
			if (Menu3_Set == 0)
			{
				Menu2_Setting_Redraw();
				Menu2_Setting_UpdateValues();
				lastSetFlag = 0xFF;
			}
		}
		if (Menu3_Set == 7)
		{
			Menu3_Set = Menu3_SetLength();
			if (Menu3_Set == 0)
			{
				Menu2_Setting_Redraw();
				Menu2_Setting_UpdateValues();
				lastSetFlag = 0xFF;
			}
		}
	}
}

int Menu2_Function(void)
{
	uint8_t FunctionFlag = 1;
	uint8_t lastFuncFlag = 0;
	uint8_t Menu2_Function = 0;
	LCD_Clear(WHITE);
	LCD_ShowChinese(0, 0, "功能", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowString(0, 20, "<---", LCD_8X16, WHITE, BLACK, 0); // 初始高亮
	LCD_ShowChinese(0, 40, "音乐播放", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 60, "秒表", LCD_16X16, BLACK, WHITE, 0);
	lastFuncFlag = 1;
	while (1)
	{
		IWDG_Feed();
		// 自动存储处理（读取实时传感器数据，原子读取标志）
		if (SaveFlag_ReadAndClear() && ToggleSaveFlag == 1)
		{
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
			uint8_t saveResult = DataStorage_New_Save(tempData, lightData, &CurrentTime);
			if (saveResult == 1)
			{
				ToggleSaveFlag = 0; // 存储已满,停止自动存储
			}
		}

		if (Encoder_Check(ENCODER_CW))
		{
			FunctionFlag--;
			if (FunctionFlag == 0)
			{
				FunctionFlag = MENU_FUNCTION_ITEM_COUNT;
			}
		}
		if (Encoder_Check(ENCODER_CCW))
		{
			FunctionFlag++;
			if (FunctionFlag == MENU_FUNCTION_ITEM_COUNT + 1)
			{
				FunctionFlag = 1;
			}
		}
		if (FunctionFlag != lastFuncFlag)
		{
			// 清除旧高亮，恢复为正常显示
			switch (lastFuncFlag)
			{
			case 1:
				LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
				break;
			case 2:
				LCD_ShowChinese(0, 40, "音乐播放", LCD_16X16, BLACK, WHITE, 0);
				break;
			case 3:
				LCD_ShowChinese(0, 60, "秒表", LCD_16X16, BLACK, WHITE, 0);
				break;
			}

			// 绘制新高亮
			switch (FunctionFlag)
			{
			case 1:
				LCD_ShowString(0, 20, "<---", LCD_8X16, WHITE, BLACK, 0);
				break;
			case 2:
				LCD_ShowChinese(0, 40, "音乐播放", LCD_16X16, WHITE, BLACK, 0);
				break;
			case 3:
				LCD_ShowChinese(0, 60, "秒表", LCD_16X16, WHITE, BLACK, 0);
				break;
			}

			lastFuncFlag = FunctionFlag;
		}

		if (Encoder_Check(ENCODER_BTN_SINGLE))
		{
			Menu2_Function = FunctionFlag;
		}
		if (Menu2_Function == 1)
		{
			return 0;
		}
		if (Menu2_Function == 2)
		{
			Menu2_Function = Menu3_Music();
			if (Menu2_Function == 0)
			{
				Menu2_Function_Redraw();
				lastFuncFlag = 0xFF;
			}
		}
		if (Menu2_Function == 3)
		{
			Menu2_Function = Menu3_StopWatch();
			if (Menu2_Function == 0)
			{
				Menu2_Function_Redraw();
				lastFuncFlag = 0xFF;
			}
		}
	}
}

int Menu3_Music(void)
{
	uint8_t CurrentMusic = 1;
	uint8_t lastPauseFlag = 0xFF;

	LCD_Clear(WHITE);
	LCD_ShowChinese(0, 0, "音乐播放", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowChinese(0, 40, "当前播放", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowChinese(0, 70, "    小星星    ", LCD_16X16, BLACK, WHITE, 1);
	LCD_DrawRectangle(1, 100, 125, 106, BLACK); // 绘制进度条边框

	Buzzer_PauseFlag = 1;
	Buzzer_FinishFlag = 0;
	Buzzer_Progress = 0;
	Buzzer_Speed = BPM2Speed(MusicBPM[CurrentMusic - 1]);

	while (1)
	{
		IWDG_Feed();
		// 1. 旋转切换歌曲
		if (Encoder_Check(ENCODER_CW))
		{
			CurrentMusic++;
			if (CurrentMusic > Music_Count)
				CurrentMusic = 1;
			goto SWITCH_SONG;
		}
		if (Encoder_Check(ENCODER_CCW))
		{
			CurrentMusic--;
			if (CurrentMusic == 0)
				CurrentMusic = Music_Count;
			goto SWITCH_SONG;
		}

		// 2. 单击播放/暂停
		if (Encoder_Check(ENCODER_BTN_SINGLE))
		{
			if (Buzzer_FinishFlag)
			{
				Buzzer_FinishFlag = 0;
				Buzzer_Progress = 0;
				Buzzer_PauseFlag = 0;
				LCD_DrawRectangle_Fill(3, 121, 124, 125, WHITE); // 清空进度条
			}
			else
			{
				Buzzer_PauseFlag = !Buzzer_PauseFlag;
				if (Buzzer_PauseFlag)
					Buzzer_OFF();
			}
		}

		// 3. 长按退出
		if (Encoder_Check(ENCODER_BTN_LONG))
		{
			Buzzer_OFF();
			Buzzer_Progress = 0;
			return 0;
		}

		// 更新播放/暂停状态显示
		if (Buzzer_PauseFlag != lastPauseFlag)
		{
			if (Buzzer_PauseFlag)
				LCD_ShowChinese(0, 120, "暂停", LCD_16X16, BLACK, WHITE, 1);
			else
				LCD_ShowChinese(0, 120, "播放", LCD_16X16, BLACK, WHITE, 1);
			lastPauseFlag = Buzzer_PauseFlag;
		}

		// 播放逻辑
		if (!Buzzer_PauseFlag && !Buzzer_FinishFlag)
		{
			switch (CurrentMusic)
			{
			case 1:
				Buzzer_Play(LittleStar);
				break;
			case 2:
				Buzzer_Play(CastleInTheSky);
				break;
			case 3:
				Buzzer_Play(Haruhikage);
				break;
			case 4:
				Buzzer_Play(Orb);
				break;
			case 5:
				Buzzer_Play(Ori);
				break;
			}
		}
		LCD_DrawRectangle_Fill(3, 102, Buzzer_Progress + 2, 104, BLUE2); // 更新进度条

		continue;

	SWITCH_SONG:
		// 切换歌曲后的公共逻辑
		Buzzer_OFF();
		Buzzer_Speed = BPM2Speed(MusicBPM[CurrentMusic - 1]);
		Buzzer_FinishFlag = 0;
		Buzzer_Progress = 0;
		Buzzer_PauseFlag = 1;							 // 切换后暂停
		LCD_DrawRectangle_Fill(3, 101, 124, 105, WHITE); // 清空进度条

		// 更新歌曲名显示
		switch (CurrentMusic)
		{
		case 1:
			LCD_ShowChinese(0, 70, "    小星星    ", LCD_16X16, BLACK, WHITE, 1);
			break;
		case 2:
			LCD_ShowChinese(0, 70, "   天空之城   ", LCD_16X16, BLACK, WHITE, 1);
			break;
		case 3:
			LCD_ShowChinese(0, 70, "    春日影    ", LCD_16X16, BLACK, WHITE, 1);
			break;
		case 4:
			LCD_ShowChinese(0, 70, "   口琴M-7    ", LCD_16X16, BLACK, WHITE, 1);
			break;
		case 5:
			LCD_ShowString(0, 70, "Light of Nibel", LCD_8X16, BLACK, WHITE, 1);
			break;
		}
	}
}

int Menu2_History(void)
{
	uint8_t Menu2_HistoryFlag = 0;
	uint8_t HisFlag = 1;
	uint8_t lastHisFlag = 0;		 // 记录上次的菜单项
	uint8_t lastRecordCount = 0;	 // 记录上次显示的记录数
	uint8_t lastStorageState = 0xFF; // 记录上次存储状态（0xFF=初始化）
	NowTotalRecords = DataStorage_New_GetCount();
	lastRecordCount = NowTotalRecords;
	LCD_Clear(WHITE);
	LCD_ShowChinese(0, 0, "历史数据", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowString(0, 20, "<---", LCD_8X16, WHITE, BLACK, 0); // 初始高亮第1项
	LCD_ShowChinese(0, 40, "读取记录", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 60, "数据图表", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 80, "擦除数据", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 100, "记录数", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowString(88, 100, "/", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowNum(96, 100, DataStorage_New_GetMaxCount(), LCD_8X16, BLACK, WHITE, 0, 1, 3);
	LCD_ShowNum(64, 100, NowTotalRecords, LCD_8X16, BLACK, WHITE, 0, 1, 3);
	lastHisFlag = 1; // 记录初始高亮项
	while (1)
	{
		IWDG_Feed();
		// 自动存储处理（读取实时传感器数据，原子读取标志）
		if (SaveFlag_ReadAndClear() && ToggleSaveFlag == 1)
		{
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
			uint8_t saveResult = DataStorage_New_Save(tempData, lightData, &CurrentTime);
			if (saveResult == 1)
			{
				ToggleSaveFlag = 0; // 存储已满，停止自动存储
			}
			// 存储后更新记录数
			NowTotalRecords = DataStorage_New_GetCount();
			if (NowTotalRecords != lastRecordCount)
			{
				LCD_ShowNum(64, 100, NowTotalRecords, LCD_8X16, BLACK, WHITE, 0, 1, 3);
				lastRecordCount = NowTotalRecords;
				lastStorageState = 0xFF; // 强制更新状态显示
			}
		}

		// 持续显示存储状态（仅在状态变化时更新）
		Menu_UpdateStorageStatus(&lastStorageState, 140);

		if (Encoder_Check(ENCODER_CW))
		{
			HisFlag--;
			if (HisFlag == 0)
			{
				HisFlag = MENU_HISTORY_ITEM_COUNT;
			}
		}
		if (Encoder_Check(ENCODER_CCW))
		{
			HisFlag++;
			if (HisFlag == MENU_HISTORY_ITEM_COUNT + 1)
			{
				HisFlag = 1;
			}
		}

		// 只在菜单项变化时更新高亮显示
		if (HisFlag != lastHisFlag)
		{
			// 清除旧项高亮（恢复为普通显示）
			switch (lastHisFlag)
			{
			case 1:
				LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
				break;
			case 2:
				LCD_ShowChinese(0, 40, "读取记录", LCD_16X16, BLACK, WHITE, 0);
				break;
			case 3:
				LCD_ShowChinese(0, 60, "数据图表", LCD_16X16, BLACK, WHITE, 0);
				break;
			case 4:
				LCD_ShowChinese(0, 80, "擦除数据", LCD_16X16, BLACK, WHITE, 0);
				break;
			}
			// 新项高亮（反色显示）
			switch (HisFlag)
			{
			case 1:
				LCD_ShowString(0, 20, "<---", LCD_8X16, WHITE, BLACK, 0);
				break;
			case 2:
				LCD_ShowChinese(0, 40, "读取记录", LCD_16X16, WHITE, BLACK, 0);
				break;
			case 3:
				LCD_ShowChinese(0, 60, "数据图表", LCD_16X16, WHITE, BLACK, 0);
				break;
			case 4:
				LCD_ShowChinese(0, 80, "擦除数据", LCD_16X16, WHITE, BLACK, 0);
				break;
			}
			lastHisFlag = HisFlag;
		}
		if (Encoder_Check(ENCODER_BTN_SINGLE))
		{
			Menu2_HistoryFlag = HisFlag;
		}
		if (Menu2_HistoryFlag == 1)
		{
			return 0;
		}
		if (Menu2_HistoryFlag == 2)
		{
			Menu2_HistoryFlag = Menu3_ReadRecord();
			if (Menu2_HistoryFlag == 0)
			{
				Menu2_History_Redraw();
				// 恢复存储状态显示
				lastStorageState = 0xFF;
				Menu_UpdateStorageStatus(&lastStorageState, 140);
				lastHisFlag = 0xFF;
				lastRecordCount = NowTotalRecords;
			}
		}
		if (Menu2_HistoryFlag == 3)
		{
			Menu2_HistoryFlag = Menu3_ShowGraph();
			if (Menu2_HistoryFlag == 0)
			{
				Menu2_History_Redraw();
				// 恢复存储状态显示
				lastStorageState = 0xFF;
				Menu_UpdateStorageStatus(&lastStorageState, 140);
				lastHisFlag = 0xFF;
				lastRecordCount = NowTotalRecords;
			}
		}
		if (Menu2_HistoryFlag == 4)
		{
			Menu2_HistoryFlag = Menu3_ChipErase();
			if (Menu2_HistoryFlag == 0)
			{
				Menu2_History_Redraw();
				// 恢复存储状态显示
				lastStorageState = 0xFF;
				Menu_UpdateStorageStatus(&lastStorageState, 140);
				lastHisFlag = 0xFF;
				lastRecordCount = NowTotalRecords;
			}
		}
	}
}

int Menu3_SetDate(void)
{
	uint8_t DateFlag = 1;
	uint8_t SwitchDateFlag = 0; // 0=Navigate, 1=Adjust
	uint8_t lastDateFlag = 0;
	uint8_t lastSwitchDateFlag = 0xFF;
	uint8_t timeChanged = 1; // Force initial draw

	LCD_Clear(WHITE);
	LCD_ShowChinese(0, 40, "日期:", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 60, "时间:", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 0, "设置", LCD_16X16, BLACK, WHITE, 1);
	// 显示日期时间分隔符
	LCD_ShowString(72, 40, "-", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowString(96, 40, "-", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowString(64, 60, ":", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowString(88, 60, ":", LCD_8X16, BLACK, WHITE, 0);

	while (1)
	{
		IWDG_Feed();
		if (SwitchDateFlag == 0) // Navigate mode
		{
			if (Encoder_Check(ENCODER_CW))
			{
				DateFlag--;
				if (DateFlag == 0)
					DateFlag = 6;
			}
			if (Encoder_Check(ENCODER_CCW))
			{
				DateFlag++;
				if (DateFlag == 7)
					DateFlag = 1;
			}
		}
		else // Adjust mode
		{
			if (Encoder_Check(ENCODER_CW))
			{
				switch (DateFlag)
				{
				case 1:
					MyRTC_Time[0]--;
					if (MyRTC_Time[0] < 2000)
						MyRTC_Time[0] = 2099;
					break;
				case 2:
					MyRTC_Time[1]--;
					if (MyRTC_Time[1] < 1)
						MyRTC_Time[1] = 12;
					break;
				case 3:
					MyRTC_Time[2]--;
					if (MyRTC_Time[2] < 1)
						MyRTC_Time[2] = 31;
					break;
				case 4:
					MyRTC_Time[3]--;
					if (MyRTC_Time[3] < 0)
						MyRTC_Time[3] = TIME_MAX_HOUR;
					break;
				case 5:
					MyRTC_Time[4]--;
					if (MyRTC_Time[4] < 0)
						MyRTC_Time[4] = TIME_MAX_MIN_SEC;
					break;
				case 6:
					MyRTC_Time[5]--;
					if (MyRTC_Time[5] < 0)
						MyRTC_Time[5] = TIME_MAX_MIN_SEC;
					break;
				}
				timeChanged = 1;
			}
			if (Encoder_Check(ENCODER_CCW))
			{
				switch (DateFlag)
				{
				case 1:
					MyRTC_Time[0]++;
					if (MyRTC_Time[0] > 2099)
						MyRTC_Time[0] = 2000;
					break;
				case 2:
					MyRTC_Time[1]++;
					if (MyRTC_Time[1] > 12)
						MyRTC_Time[1] = 1;
					break;
				case 3:
					MyRTC_Time[2]++;
					if (MyRTC_Time[2] > 31)
						MyRTC_Time[2] = 1;
					break;
				case 4:
					MyRTC_Time[3]++;
					if (MyRTC_Time[3] > TIME_MAX_HOUR)
						MyRTC_Time[3] = 0;
					break;
				case 5:
					MyRTC_Time[4]++;
					if (MyRTC_Time[4] > TIME_MAX_MIN_SEC)
						MyRTC_Time[4] = 0;
					break;
				case 6:
					MyRTC_Time[5]++;
					if (MyRTC_Time[5] > TIME_MAX_MIN_SEC)
						MyRTC_Time[5] = 0;
					break;
				}
				timeChanged = 1;
			}
		}

		if (Encoder_Check(ENCODER_BTN_SINGLE))
		{
			SwitchDateFlag = !SwitchDateFlag;
		}
		else if (Encoder_Check(ENCODER_BTN_LONG))
		{
			LCD_Clear(WHITE);
			LCD_ShowChinese(0, 0, "设置", LCD_16X16, BLACK, WHITE, 1);
			MyRTC_SetTime();
			return 0;
		}

		// Update Display
		if (DateFlag != lastDateFlag || timeChanged)
		{
			uint16_t fc, bc;

			// Year
			fc = (DateFlag == 1) ? WHITE : BLACK;
			bc = (DateFlag == 1) ? BLACK : WHITE;
			LCD_ShowNum(40, 40, MyRTC_Time[0], LCD_8X16, fc, bc, 0, 0, 4);

			// Month
			fc = (DateFlag == 2) ? WHITE : BLACK;
			bc = (DateFlag == 2) ? BLACK : WHITE;
			LCD_ShowNum(80, 40, MyRTC_Time[1], LCD_8X16, fc, bc, 0, 1, 2);

			// Day
			fc = (DateFlag == 3) ? WHITE : BLACK;
			bc = (DateFlag == 3) ? BLACK : WHITE;
			LCD_ShowNum(104, 40, MyRTC_Time[2], LCD_8X16, fc, bc, 0, 1, 2);

			// Hour
			fc = (DateFlag == 4) ? WHITE : BLACK;
			bc = (DateFlag == 4) ? BLACK : WHITE;
			LCD_ShowNum(48, 60, MyRTC_Time[3], LCD_8X16, fc, bc, 0, 1, 2);

			// Minute
			fc = (DateFlag == 5) ? WHITE : BLACK;
			bc = (DateFlag == 5) ? BLACK : WHITE;
			LCD_ShowNum(72, 60, MyRTC_Time[4], LCD_8X16, fc, bc, 0, 1, 2);

			// Second
			fc = (DateFlag == 6) ? WHITE : BLACK;
			bc = (DateFlag == 6) ? BLACK : WHITE;
			LCD_ShowNum(96, 60, MyRTC_Time[5], LCD_8X16, fc, bc, 0, 1, 2);

			lastDateFlag = DateFlag;
			timeChanged = 0;
		}

		if (SwitchDateFlag != lastSwitchDateFlag)
		{
			if (SwitchDateFlag == 0)
				LCD_ShowChinese(0, 120, "切换", LCD_16X16, BLACK, WHITE, 1);
			else
				LCD_ShowChinese(0, 120, "调整", LCD_16X16, BLACK, WHITE, 1);
			lastSwitchDateFlag = SwitchDateFlag;
		}
	}
}

int Menu3_SetLight(void)
{
	static uint16_t LastLightValue = 0xFFFF;
	LCD_Clear(WHITE);
	LCD_ShowChinese(0, 0, "设置", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 40, "日期时间", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 60, "自动存储", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 80, "光照阈值", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 100, "温度阈值", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 120, "最大记录", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 140, "单次步长", LCD_16X16, BLACK, WHITE, 0);

	LCD_ShowNum(72, 80, LightValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
	LCD_ShowSignedNum(72, 100, TempValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
	LCD_ShowNum(72, 120, DataStorage_New_GetMaxCount(), LCD_8X16, BLACK, WHITE, 0, 0, 0);
	LCD_ShowNum(72, 140, StepLength, LCD_8X16, BLACK, WHITE, 0, 0, 0);

	LCD_ShowString(64, 80, ">", LCD_8X16, WHITE, BLACK, 0);
	LastLightValue = LightValue;

	while (1)
	{
		IWDG_Feed();
		if (Encoder_Check(ENCODER_CW))
		{
			LightValue = LightValue - StepLength;
			if (LightValue <= 0)
			{
				LightValue = 999;
			}
		}
		if (Encoder_Check(ENCODER_CCW))
		{
			LightValue = LightValue + StepLength;
			if (LightValue >= 999)
			{
				LightValue = 0;
			}
		}
		if (Encoder_Check(ENCODER_BTN_SINGLE))
		{
			LCD_Clear(WHITE);
			LastLightValue = 0xFFFF;
			return 0;
		}

		if (LightValue != LastLightValue)
		{
			LCD_DrawRectangle_Fill(72, 80, 110, 95, WHITE);
			LCD_ShowNum(72, 80, LightValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
			LastLightValue = LightValue;
		}
	}
}

int Menu3_SetTemp(void)
{
	static int LastTempValue = 0xFF;
	LCD_Clear(WHITE);
	LCD_ShowChinese(0, 0, "设置", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 40, "日期时间", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 60, "自动存储", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 80, "光照阈值", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 100, "温度阈值", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 120, "最大记录", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 140, "单次步长", LCD_16X16, BLACK, WHITE, 0);

	LCD_ShowNum(72, 80, LightValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
	LCD_ShowSignedNum(72, 100, TempValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
	LCD_ShowNum(72, 120, DataStorage_New_GetMaxCount(), LCD_8X16, BLACK, WHITE, 0, 0, 0);
	LCD_ShowNum(72, 140, StepLength, LCD_8X16, BLACK, WHITE, 0, 0, 0);

	LCD_ShowString(64, 100, ">", LCD_8X16, WHITE, BLACK, 0);
	LastTempValue = TempValue;

	while (1)
	{
		IWDG_Feed();
		if (Encoder_Check(ENCODER_CW))
		{
			TempValue = TempValue - StepLength;
			if (TempValue <= -40)
			{
				TempValue = 115;
			}
		}
		if (Encoder_Check(ENCODER_CCW))
		{
			TempValue = TempValue + StepLength;
			if (TempValue >= 115)
			{
				TempValue = -40;
			}
		}
		if (Encoder_Check(ENCODER_BTN_SINGLE))
		{
			LCD_Clear(WHITE);
			LastTempValue = 0xFF;
			return 0;
		}

		if (TempValue != LastTempValue)
		{
			LCD_DrawRectangle_Fill(72, 100, 110, 115, WHITE);
			LCD_ShowSignedNum(72, 100, TempValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
			LastTempValue = TempValue;
		}
	}
}

int Menu3_SetLength(void)
{
	static uint8_t LastStepLength = 0xFF;
	LCD_Clear(WHITE);
	LCD_ShowChinese(0, 0, "设置", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 40, "日期时间", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 60, "自动存储", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 80, "光照阈值", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 100, "温度阈值", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 120, "最大记录", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 140, "单次步长", LCD_16X16, BLACK, WHITE, 0);

	LCD_ShowNum(72, 80, LightValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
	LCD_ShowSignedNum(72, 100, TempValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
	LCD_ShowNum(72, 120, DataStorage_New_GetMaxCount(), LCD_8X16, BLACK, WHITE, 0, 0, 0);
	LCD_ShowNum(72, 140, StepLength, LCD_8X16, BLACK, WHITE, 0, 0, 0);

	LCD_ShowString(64, 140, ">", LCD_8X16, WHITE, BLACK, 0);
	LastStepLength = StepLength;

	while (1)
	{
		IWDG_Feed();
		if (Encoder_Check(ENCODER_CW))
		{
			StepLength--;
			if (StepLength == 0)
			{
				StepLength = 100;
			}
		}
		if (Encoder_Check(ENCODER_CCW))
		{
			StepLength++;
			if (StepLength == 101)
			{
				StepLength = 0;
			}
		}
		if (Encoder_Check(ENCODER_BTN_SINGLE))
		{
			LCD_Clear(WHITE);
			LastStepLength = 0xFF;
			return 0;
		}

		if (StepLength != LastStepLength)
		{
			LCD_DrawRectangle_Fill(72, 140, 110, 155, WHITE);
			LCD_ShowNum(72, 140, StepLength, LCD_8X16, BLACK, WHITE, 0, 0, 0);
			LastStepLength = StepLength;
		}
	}
}

int Menu3_AutoSaveSet(void)
{
	uint8_t AutoSaveFlag = 1;
	uint8_t lastAutoSaveFlag = 0;
	uint8_t Menu3_ToggleSaveFlag = 0;
	uint8_t lastStorageState = 0xFF;

	// 初始绘制界面
	LCD_Clear(WHITE);
	LCD_ShowChinese(0, 0, "设置", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowString(0, 20, "<---", LCD_8X16, WHITE, BLACK, 0); // 默认选中第一项
	LCD_ShowChinese(0, 40, "自动存储", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 60, "存储间隔", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowString(40, 100, ":", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowString(80, 100, ":", LCD_8X16, BLACK, WHITE, 0);

	// 显示初始值
	SaveInterval_Time[0] = SaveInterval / (TIME_MINUTES_PER_HOUR * TIME_SECONDS_PER_MINUTE);
	SaveInterval_Time[1] = (SaveInterval % (TIME_MINUTES_PER_HOUR * TIME_SECONDS_PER_MINUTE)) / TIME_SECONDS_PER_MINUTE;
	SaveInterval_Time[2] = (SaveInterval % (TIME_MINUTES_PER_HOUR * TIME_SECONDS_PER_MINUTE)) % TIME_SECONDS_PER_MINUTE;
	LCD_ShowNum(16, 100, SaveInterval_Time[0], LCD_8X16, BLACK, WHITE, 0, 1, 2);
	LCD_ShowNum(56, 100, SaveInterval_Time[1], LCD_8X16, BLACK, WHITE, 0, 1, 2);
	LCD_ShowNum(96, 100, SaveInterval_Time[2], LCD_8X16, BLACK, WHITE, 0, 1, 2);

	if (ToggleSaveFlag == 0)
		LCD_ShowChinese(80, 40, "关", LCD_16X16, BLACK, WHITE, 0);
	else
		LCD_ShowChinese(80, 40, "开", LCD_16X16, BLACK, WHITE, 0);

	lastAutoSaveFlag = 1;

	while (1)
	{
		IWDG_Feed();
		if (Encoder_Check(ENCODER_CW))
		{
			AutoSaveFlag--;
			if (AutoSaveFlag == 0)
				AutoSaveFlag = 3;
		}
		if (Encoder_Check(ENCODER_CCW))
		{
			AutoSaveFlag++;
			if (AutoSaveFlag == 4)
				AutoSaveFlag = 1;
		}

		// 更新高亮
		if (AutoSaveFlag != lastAutoSaveFlag)
		{
			// 清除旧高亮
			switch (lastAutoSaveFlag)
			{
			case 1:
				LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
				break;
			case 2:
				LCD_ShowChinese(0, 40, "自动存储", LCD_16X16, BLACK, WHITE, 0);
				break;
			case 3:
				LCD_ShowChinese(0, 60, "存储间隔", LCD_16X16, BLACK, WHITE, 0);
				break;
			}
			// 设置新高亮
			switch (AutoSaveFlag)
			{
			case 1:
				LCD_ShowString(0, 20, "<---", LCD_8X16, WHITE, BLACK, 0);
				break;
			case 2:
				LCD_ShowChinese(0, 40, "自动存储", LCD_16X16, WHITE, BLACK, 0);
				break;
			case 3:
				LCD_ShowChinese(0, 60, "存储间隔", LCD_16X16, WHITE, BLACK, 0);
				break;
			}
			lastAutoSaveFlag = AutoSaveFlag;
		}

		if (Encoder_Check(ENCODER_BTN_SINGLE))
		{
			Menu3_ToggleSaveFlag = AutoSaveFlag;
		}

		if (Menu3_ToggleSaveFlag == 1)
		{
			return 0;
		}

		// 处理子菜单返回
		if (Menu3_ToggleSaveFlag == 2 || Menu3_ToggleSaveFlag == 3)
		{
			if (Menu3_ToggleSaveFlag == 2)
				Menu3_ToggleSaveFlag = Menu4_ToggleSave();
			else
				Menu3_ToggleSaveFlag = Menu4_SaveInterval();

			// 返回后重绘界面
			LCD_Clear(WHITE);
			LCD_ShowChinese(0, 0, "设置", LCD_16X16, BLACK, WHITE, 1);
			LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 40, "自动存储", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowChinese(0, 60, "存储间隔", LCD_16X16, BLACK, WHITE, 0);
			LCD_ShowString(40, 100, ":", LCD_8X16, BLACK, WHITE, 0);
			LCD_ShowString(80, 100, ":", LCD_8X16, BLACK, WHITE, 0);

			SaveInterval_Time[0] = SaveInterval / (TIME_MINUTES_PER_HOUR * TIME_SECONDS_PER_MINUTE);
			SaveInterval_Time[1] = (SaveInterval % (TIME_MINUTES_PER_HOUR * TIME_SECONDS_PER_MINUTE)) / TIME_SECONDS_PER_MINUTE;
			SaveInterval_Time[2] = (SaveInterval % (TIME_MINUTES_PER_HOUR * TIME_SECONDS_PER_MINUTE)) % TIME_SECONDS_PER_MINUTE;
			LCD_ShowNum(16, 100, SaveInterval_Time[0], LCD_8X16, BLACK, WHITE, 0, 1, 2);
			LCD_ShowNum(56, 100, SaveInterval_Time[1], LCD_8X16, BLACK, WHITE, 0, 1, 2);
			LCD_ShowNum(96, 100, SaveInterval_Time[2], LCD_8X16, BLACK, WHITE, 0, 1, 2);

			if (ToggleSaveFlag == 0)
				LCD_ShowChinese(80, 40, "关", LCD_16X16, BLACK, WHITE, 0);
			else
				LCD_ShowChinese(80, 40, "开", LCD_16X16, BLACK, WHITE, 0);

			lastAutoSaveFlag = 0xFF; // 强制刷新高亮
			lastStorageState = 0xFF; // 强制刷新状态
		}

		// 自动存储逻辑（原子读取标志）
		if (SaveFlag_ReadAndClear() && ToggleSaveFlag == 1)
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
			uint8_t saveResult = DataStorage_New_Save(Temp, Light, &CurrentTime);
			if (saveResult == 1)
			{
				ToggleSaveFlag = 0; // 存储已满，停止自动存储
			}
		}

		// 持续显示存储状态（使用统一的更新函数）
		Menu_UpdateStorageStatus(&lastStorageState, 140);
	}
}

int Menu3_ReadRecord(void)
{
	uint8_t ReadRecordFlag = 1;
	uint8_t Menu3_ReadRecordFlag = 0;
	uint8_t RefreshFlag = 1;
	NowTotalRecords = DataStorage_New_GetCount();
	if (NowTotalRecords == 0)
	{
		LCD_Clear(WHITE);
		LCD_ShowChinese(16, 60, "暂无记录", LCD_16X16, RED, WHITE, 1);
		Delay_ms(MENU_DELAY_NO_DATA_MS);
		return 0;
	}
	LCD_Clear(WHITE);
	LCD_ShowChinese(0, 0, "历史数据", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowString(0, 20, "<---", LCD_8X16, WHITE, BLACK, 0);
	LCD_ShowChinese(0, 80, "光照(lux):", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 40, "日期:", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 60, "时间:", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 100, "温度(℃):", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowString(0, 120, "ID:", LCD_8X16, BLACK, WHITE, 0);

	while (1)
	{
		IWDG_Feed();
		NowTotalRecords = DataStorage_New_GetCount();
		if (Encoder_Check(ENCODER_CW))
		{
			if (ShowRecordID > 0)
			{
				ShowRecordID--;
			}
			RefreshFlag = 1;
		}
		if (Encoder_Check(ENCODER_CCW))
		{
			if (ShowRecordID < NowTotalRecords - 1)
			{
				ShowRecordID++;
			}
			RefreshFlag = 1;
		}
		if (Encoder_Check(ENCODER_BTN_SINGLE))
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
	uint8_t lastEraseFlag = 0;

	LCD_ShowChinese(0, 40, "确认擦除?", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowChinese(20, 72, "是", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(88, 72, "否", LCD_16X16, BLACK, WHITE, 0);

	// 初始高亮"否"
	LCD_ShowChinese(88, 72, "否", LCD_16X16, WHITE, BLACK, 0);
	lastEraseFlag = 2;

	while (1)
	{
		IWDG_Feed();
		if (Encoder_Check(ENCODER_CW))
		{
			EraseFlag--;
			if (EraseFlag == 0)
				EraseFlag = 2;
		}
		if (Encoder_Check(ENCODER_CCW))
		{
			EraseFlag++;
			if (EraseFlag == 3)
				EraseFlag = 1;
		}

		if (EraseFlag != lastEraseFlag)
		{
			if (EraseFlag == 1)
			{
				LCD_ShowChinese(20, 72, "是", LCD_16X16, WHITE, BLACK, 0);
				LCD_ShowChinese(88, 72, "否", LCD_16X16, BLACK, WHITE, 0);
			}
			else
			{
				LCD_ShowChinese(20, 72, "是", LCD_16X16, BLACK, WHITE, 0);
				LCD_ShowChinese(88, 72, "否", LCD_16X16, WHITE, BLACK, 0);
			}
			lastEraseFlag = EraseFlag;
		}

		if (Encoder_Check(ENCODER_BTN_SINGLE))
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
	}
}

int Menu3_SetMaxRecord(void)
{
	uint8_t tempMax = DataStorage_New_GetMaxCount();
	uint8_t lastTempMax = 0;

	LCD_Clear(WHITE);
	LCD_ShowChinese(0, 0, "设置", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 40, "日期时间", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 60, "自动存储", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 80, "光照阈值", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 100, "温度阈值", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 120, "最大记录", LCD_16X16, BLACK, WHITE, 0);
	LCD_ShowChinese(0, 140, "单次步长", LCD_16X16, BLACK, WHITE, 0);

	LCD_ShowNum(72, 80, LightValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
	LCD_ShowSignedNum(72, 100, TempValue, LCD_8X16, BLACK, WHITE, 0, 0, 0);
	LCD_ShowNum(72, 120, tempMax, LCD_8X16, BLACK, WHITE, 0, 0, 0);
	LCD_ShowNum(72, 140, StepLength, LCD_8X16, BLACK, WHITE, 0, 0, 0);

	LCD_ShowString(64, 120, ">", LCD_8X16, WHITE, BLACK, 0);
	lastTempMax = tempMax;

	while (1)
	{
		IWDG_Feed();
		if (ToggleSaveFlag == 0)
		{
			if (Encoder_Check(ENCODER_CW))
			{
				if (tempMax > StepLength)
					tempMax -= StepLength;
				else
					tempMax = 250;
			}
			if (Encoder_Check(ENCODER_CCW))
			{
				if (tempMax + StepLength <= 250)
					tempMax += StepLength;
				else
					tempMax = 1;
			}
			if (Encoder_Check(ENCODER_BTN_SINGLE))
			{
				DataStorage_New_SetMaxCount(tempMax);
				LCD_Clear(WHITE);
				return 0;
			}
		}

		if (tempMax != lastTempMax)
		{
			LCD_DrawRectangle_Fill(72, 120, 110, 135, WHITE);
			LCD_ShowNum(72, 120, tempMax, LCD_8X16, BLACK, WHITE, 0, 0, 0);
			lastTempMax = tempMax;
		}
	}
}

int Menu4_Erase(void)
{
	LCD_Clear(WHITE);
	LCD_ShowChinese(0, 40, "擦除中...", LCD_16X16, RED, WHITE, 1);
	DataStorage_New_EraseAll();
	NowTotalRecords = DataStorage_New_GetCount();
	Delay_s(MENU_DELAY_ERASE_SEC);
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
	uint8_t lastStopWatchFlag = 0;
	uint8_t StopWatchTemp = 0;
	static uint8_t LastHour = 0xFF, LastMinute = 0xFF, LastSecond = 0xFF;

	LCD_Clear(WHITE);
	LCD_ShowChinese(0, 0, "秒表", LCD_16X16, BLACK, WHITE, 1);
	LCD_ShowString(0, 20, "<---", LCD_8X16, WHITE, BLACK, 0); // 初始高亮
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
	lastStopWatchFlag = 1;

	while (1)
	{
		IWDG_Feed();
		if (Encoder_Check(ENCODER_CW))
		{
			StopWatchFlag--;
			if (StopWatchFlag == 0)
			{
				StopWatchFlag = 4;
			}
		}
		if (Encoder_Check(ENCODER_CCW))
		{
			StopWatchFlag++;
			if (StopWatchFlag == 5)
			{
				StopWatchFlag = 1;
			}
		}
		if (Encoder_Check(ENCODER_BTN_SINGLE))
		{
			StopWatchTemp = StopWatchFlag;
		}
		if (StopWatchTemp == 1)
		{
			LastHour = LastMinute = LastSecond = 0xFF;
			StopWatchStartFlag = 0;
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

		// 增量更新菜单高亮（仅在变化时刷新）
		if (StopWatchFlag != lastStopWatchFlag)
		{
			// 清除旧高亮
			switch (lastStopWatchFlag)
			{
			case 1:
				LCD_ShowString(0, 20, "<---", LCD_8X16, BLACK, WHITE, 0);
				break;
			case 2:
				LCD_ShowChinese(8, 100, "开始", LCD_16X16, BLACK, WHITE, 0);
				break;
			case 3:
				LCD_ShowChinese(48, 100, "暂停", LCD_16X16, BLACK, WHITE, 0);
				break;
			case 4:
				LCD_ShowChinese(88, 100, "清除", LCD_16X16, BLACK, WHITE, 0);
				break;
			}

			// 绘制新高亮
			switch (StopWatchFlag)
			{
			case 1:
				LCD_ShowString(0, 20, "<---", LCD_8X16, WHITE, BLACK, 0);
				break;
			case 2:
				LCD_ShowChinese(8, 100, "开始", LCD_16X16, WHITE, BLACK, 0);
				break;
			case 3:
				LCD_ShowChinese(48, 100, "暂停", LCD_16X16, WHITE, BLACK, 0);
				break;
			case 4:
				LCD_ShowChinese(88, 100, "清除", LCD_16X16, WHITE, BLACK, 0);
				break;
			}

			lastStopWatchFlag = StopWatchFlag;
		}
	}
}

int Menu3_ShowGraph(void)
{
	uint8_t totalRecords = DataStorage_New_GetCount();

	LCD_Clear(WHITE);
	// 如果没有记录，显示提示信息并退出
	if (totalRecords == 0)
	{
		LCD_ShowChinese(16, 60, "暂无数据", LCD_16X16, RED, WHITE, 1);
		Delay_ms(MENU_DELAY_NO_DATA_MS);
		return 0;
	}

	uint8_t chartType = 0; // 0=温度, 1=光照
	int16_t offset = 0;	   // 图表偏移量（向右滚动为正）
	uint8_t needRedraw = 1;
	uint8_t fullRedraw = 1; // 首次需要全屏刷新

	// 首次绘制
	DrawStorageChart(chartType, offset, totalRecords, fullRedraw);
	fullRedraw = 0; // 后续只需局部刷新

	while (1)
	{
		IWDG_Feed();
		// KEY1: 向左移动
		if (Encoder_Check(ENCODER_CW))
		{
			if (offset > 0)
			{
				offset--;
				needRedraw = 1;
			}
		}

		// KEY2: 向右移动
		if (Encoder_Check(ENCODER_CCW))
		{
			// 最大偏移量为总记录数减去屏幕显示点数
			int16_t maxOffset = totalRecords - 25;
			if (maxOffset < 0)
				maxOffset = 0;
			if (offset < maxOffset)
			{
				offset++;
				needRedraw = 1;
			}
		}

		// KEY3单击: 切换温度/光照
		if (Encoder_Check(ENCODER_BTN_SINGLE))
		{
			chartType = 1 - chartType;
			needRedraw = 1;
		}

		// KEY3长按: 退出
		if (Encoder_Check(ENCODER_BTN_LONG))
		{
			return 0;
		}

		// 重绘图表
		if (needRedraw)
		{
			needRedraw = 0;
			DrawStorageChart(chartType, offset, totalRecords, fullRedraw);
		}

		Delay_ms(10);
	}
}

// 绘制存储数据图表
void DrawStorageChart(uint8_t chartType, int16_t offset, uint8_t totalRecords, uint8_t fullRedraw)
{
	if (fullRedraw)
	{
		LCD_Clear(WHITE);
	}
	else
	{
		// 局部清除：清除图表区域
		LCD_DrawRectangle_Fill(0, 44, LCD_WIDTH - 1, LCD_HEIGHT - 1, WHITE);
		// 局部清除：清除顶部数值区域 (Max/Min值)
		LCD_DrawRectangle_Fill(24, 20, 60, 40, WHITE);
		// 局部清除：清除右上角记录信息
		LCD_DrawRectangle_Fill(60, 24, LCD_WIDTH - 1, 32, WHITE);
	}

	// 显示标题和类型
	if (chartType == 0)
	{
		LCD_ShowChinese(0, 0, "温度曲线", LCD_16X16, BLACK, WHITE, 1);
	}
	else
	{
		LCD_ShowChinese(0, 0, "光照曲线", LCD_16X16, BLACK, WHITE, 1);
	}

	// 显示记录范围信息
	char info[20];
	uint8_t startRecord = offset + 1;
	uint8_t endRecord = offset + 25;
	if (endRecord > totalRecords)
		endRecord = totalRecords;
	sprintf(info, "%d-%d/%d", startRecord, endRecord, totalRecords);
	LCD_ShowString(60, 24, info, LCD_6X8, BLUE, WHITE, 0);

	// 计算实际要显示的数据点数
	uint8_t displayCount = totalRecords - offset;
	if (displayCount > 25)
		displayCount = 25;

	// 如果没有数据可显示
	if (displayCount == 0)
	{
		LCD_ShowChinese(32, 60, "无数据", LCD_16X16, RED, WHITE, 1);
		return;
	}

	// 读取数据并计算最大最小值
	int16_t dataMax = -32768, dataMin = 32767;
	SensorData_New SensorData_New;

	// 第一遍扫描：找出最大最小值
	for (uint8_t i = 0; i < displayCount; i++)
	{
		if (i % 10 == 0)
			IWDG_Feed(); // 每10个点喂一次狗

		uint8_t index = offset + i; // 记录索引从0开始

		if (DataStorage_New_Read(index, &SensorData_New) == 0)
		{
			int16_t value;
			if (chartType == 0)
			{
				value = SensorData_New.Temp; // 温度值已经×10
			}
			else
			{
				value = (int16_t)SensorData_New.Light;
			}

			if (value > dataMax)
				dataMax = value;
			if (value < dataMin)
				dataMin = value;
		}
	}

	// 防止最大最小值相同或范围过小，添加合理边距
	int16_t dataRange = dataMax - dataMin;

	if (chartType == 0)
	{
		// 温度图表：如果范围小于5℃（50个单位），扩展到至少5℃范围
		if (dataRange < 50)
		{
			int16_t center = (dataMax + dataMin) / 2;
			dataMax = center + 25; // +2.5℃
			dataMin = center - 25; // -2.5℃
		}
		else
		{
			// 添加10%的上下边距，让曲线更美观
			int16_t margin = dataRange / 10;
			if (margin < 5)
				margin = 5; // 至少0.5℃边距
			dataMax += margin;
			dataMin -= margin;
		}
	}
	else
	{
		// 光照图表：如果范围小于200 lux，扩展到至少200范围
		if (dataRange < 200)
		{
			int16_t center = (dataMax + dataMin) / 2;
			dataMax = center + 100; // +100 lux
			dataMin = center - 100; // -100 lux
			// 确保不小于0
			if (dataMin < 0)
				dataMin = 0;
			// 如果调整后dataMin=0，相应调整dataMax保持范围
			if (dataMin == 0 && center < 100)
			{
				dataMax = 200;
			}
		}
		else
		{
			// 添加20%的上下边距（光照变化通常较大）
			int16_t margin = dataRange / 5;
			if (margin < 20)
				margin = 20; // 至少20 lux边距
			dataMax += margin;
			if (dataMin > margin)
				dataMin -= margin;
			else
				dataMin = 0;
		}
	}

	// 重新计算范围
	dataRange = dataMax - dataMin;

	// 显示最大最小值
	LCD_ShowString(0, 20, "Max:", LCD_6X8, BLACK, WHITE, 0);
	LCD_ShowString(0, 30, "Min:", LCD_6X8, BLACK, WHITE, 0);

	if (chartType == 0)
	{
		LCD_ShowFloatNum(24, 20, dataMax / 10.0f, 1, LCD_6X8, RED, WHITE, 0, 0);
		LCD_ShowFloatNum(24, 30, dataMin / 10.0f, 1, LCD_6X8, BLUE, WHITE, 0, 0);
	}
	else
	{
		LCD_ShowNum(24, 20, dataMax, LCD_6X8, RED, WHITE, 0, 0, 0);
		LCD_ShowNum(24, 30, dataMin, LCD_6X8, BLUE, WHITE, 0, 0, 0);
	}

	// 第二遍扫描：绘制曲线
	int16_t prevX = -1, prevY = -1;
	uint8_t pixelStep = 5; // 每个点间隔5像素

	for (uint8_t i = 0; i < displayCount; i++)
	{
		uint8_t index = offset + i; // 记录索引从0开始

		if (DataStorage_New_Read(index, &SensorData_New) == 0)
		{
			int16_t value;
			if (chartType == 0)
			{
				value = SensorData_New.Temp;
			}
			else
			{
				value = (int16_t)SensorData_New.Light;
			}

			// 计算坐标（使用更新后的dataRange）
			int16_t x = 2 + i * pixelStep;
			int16_t y = 149 - ((int32_t)(value - dataMin) * 105 / dataRange);

			// 确保Y坐标在有效范围内
			if (y < 44)
				y = 44;
			if (y > 149)
				y = 149;

			// 绘制点
			uint16_t color = (chartType == 0) ? RED : BLUE;
			LCD_DrawRectangle_Fill(x - 1, y - 1, x + 1, y + 1, color);

			// 绘制连线
			if (i > 0 && prevX >= 0)
			{
				LCD_DrawLine(prevX, prevY, x, y, color);
			}

			prevX = x;
			prevY = y;
		}
	}
}

int Menu4_ToggleSave(void)
{
	uint8_t showWarning = 0; // 显示警告标志
	LCD_ShowChinese(0, 0, "历史数据", LCD_16X16, BLACK, WHITE, 1);
	SaveInterval_Time[0] = SaveInterval / (TIME_MINUTES_PER_HOUR * TIME_SECONDS_PER_MINUTE);
	SaveInterval_Time[1] = (SaveInterval % (TIME_MINUTES_PER_HOUR * TIME_SECONDS_PER_MINUTE)) / TIME_SECONDS_PER_MINUTE;
	SaveInterval_Time[2] = (SaveInterval % (TIME_MINUTES_PER_HOUR * TIME_SECONDS_PER_MINUTE)) % TIME_SECONDS_PER_MINUTE;
	LCD_ShowString(40, 100, ":", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowString(80, 100, ":", LCD_8X16, BLACK, WHITE, 0);
	LCD_ShowNum(16, 100, SaveInterval_Time[0], LCD_8X16, BLACK, WHITE, 0, 1, 2);
	LCD_ShowNum(56, 100, SaveInterval_Time[1], LCD_8X16, BLACK, WHITE, 0, 1, 2);
	LCD_ShowNum(96, 100, SaveInterval_Time[2], LCD_8X16, BLACK, WHITE, 0, 1, 2);
	while (1)
	{
		IWDG_Feed();
		if (Encoder_Check(ENCODER_CW))
		{
			// 检查是否尝试从关闭切换到开启
			if (ToggleSaveFlag == 0)
			{
				// 检查存储是否已满
				uint8_t currentCount = DataStorage_New_GetCount();
				if (currentCount >= DataStorage_New_GetMaxCount())
				{
					showWarning = 1; // 显示警告
				}
				else
				{
					ToggleSaveFlag = 1; // 允许开启
					showWarning = 0;
				}
			}
			else
			{
				ToggleSaveFlag = 0; // 允许关闭
				showWarning = 0;
			}
		}
		if (Encoder_Check(ENCODER_CCW))
		{
			// 检查是否尝试从关闭切换到开启
			if (ToggleSaveFlag == 0)
			{
				// 检查存储是否已满
				uint8_t currentCount = DataStorage_New_GetCount();
				if (currentCount >= DataStorage_New_GetMaxCount())
				{
					showWarning = 1; // 显示警告
				}
				else
				{
					ToggleSaveFlag = 1; // 允许开启
					showWarning = 0;
				}
			}
			else
			{
				ToggleSaveFlag = 0; // 允许关闭
				showWarning = 0;
			}
		}
		if (Encoder_Check(ENCODER_BTN_SINGLE))
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

		// 显示警告信息
		if (showWarning)
		{
			LCD_ShowChinese(0, 120, "存储已满", LCD_16X16, RED, WHITE, 1);
			Delay_s(1);
			showWarning = 0; // 只显示一次警告
		}
		else
		{
			// 清除警告区域
			LCD_ShowString(0, 120, "                ", LCD_8X16, WHITE, WHITE, 0);
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
	SaveInterval_Time[0] = SaveInterval / (TIME_MINUTES_PER_HOUR * TIME_SECONDS_PER_MINUTE);
	SaveInterval_Time[1] = (SaveInterval % (TIME_MINUTES_PER_HOUR * TIME_SECONDS_PER_MINUTE)) / TIME_SECONDS_PER_MINUTE;
	SaveInterval_Time[2] = (SaveInterval % (TIME_MINUTES_PER_HOUR * TIME_SECONDS_PER_MINUTE)) % TIME_SECONDS_PER_MINUTE;
	while (1)
	{
		IWDG_Feed();
		if (SwitchFlag == 0)
		{
			if (Encoder_Check(ENCODER_CW)) // KEY_1：上一项（小时→秒循环）
			{
				SaveIntervalFlag--;
				if (SaveIntervalFlag == 0)
				{
					SaveIntervalFlag = 3;
				}
			}
			else if (Encoder_Check(ENCODER_CCW)) // KEY_2：下一项（秒→小时循环）
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
			if (Encoder_Check(ENCODER_CW)) // KEY_1：减少
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
				SaveInterval = SaveInterval_Time[0] * (TIME_MINUTES_PER_HOUR * TIME_SECONDS_PER_MINUTE) + SaveInterval_Time[1] * TIME_SECONDS_PER_MINUTE + SaveInterval_Time[2];
				// 防止间隔为0，最小1秒
				if (SaveInterval == 0)
				{
					SaveInterval = 1;
					SaveInterval_Time[2] = 1;
				}
			}
			else if (Encoder_Check(ENCODER_CCW)) // KEY_2：增加
			{
				switch (SaveIntervalFlag)
				{
				case 1:
				{
					SaveInterval_Time[0]++;
					if (SaveInterval_Time[0] == TIME_HOURS_PER_DAY)
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
				SaveInterval = SaveInterval_Time[0] * (TIME_MINUTES_PER_HOUR * TIME_SECONDS_PER_MINUTE) + SaveInterval_Time[1] * TIME_SECONDS_PER_MINUTE + SaveInterval_Time[2];
				// 防止间隔为0，最小1秒
				if (SaveInterval == 0)
				{
					SaveInterval = 1;
					SaveInterval_Time[2] = 1;
				}
			}
		}
		if (Encoder_Check(ENCODER_BTN_SINGLE))
		{
			SwitchFlag = !SwitchFlag;
		}
		else if (Encoder_Check(ENCODER_BTN_LONG))
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
	SensorData_New record;
	NowTotalRecords = DataStorage_New_GetCount();
	if (RecordID >= NowTotalRecords)
	{
		LCD_ShowChinese(32, 120, "无记录", LCD_16X16, BLACK, WHITE, 0);
		return;
	}

	// 读取记录并检查是否成功
	if (DataStorage_New_Read(RecordID, &record) != 0)
	{
		LCD_ShowChinese(32, 120, "读取错误", LCD_16X16, RED, WHITE, 0);
		return;
	}

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
	LCD_ShowNum(88, 80, record.Light, LCD_8X16, BLACK, WHITE, 0, 1, 3);	 // 光照，固定3位
	LCD_ShowFloatNum(72, 100, temp, 1, LCD_8X16, BLACK, WHITE, 0, 6);	 // 温度，固定6字符宽度

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

/**
 * @brief  TIM4中断处理函数
 * @note   中断周期: 1ms
 *         功能: 按键防抖、秒表计时、蜂鸣器控制、编码器检测
 */
void TIM4_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) == SET)
	{
		SystemTick++; // 增加系统滴答计数
		Key_Tick();
		StopWatch_Tick();
		Buzzer_Tick();
		Encoder_Tick(); // 编码器按键防抖和长按检测(1ms周期)
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
	}
}

/**
 * @brief  TIM2中断处理函数 - 存储间隔计时
 * @note   中断周期: 1秒
 *         功能: 累计计数达到 SaveInterval 时触发存储标志
 */
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		TimerCount++;
		if (TimerCount >= SaveInterval)
		{
			TimerCount = 0;
			SaveFlag = 1; // 通知主循环执行存储（原子写入）
		}
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}
