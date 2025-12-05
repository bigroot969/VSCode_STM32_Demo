#ifndef __MENU_H
#define __MENU_H
#include "stm32f10x.h" // Device header
#include "LCD.h"
#include "Key.h"
#include "Menu.h"
#include "MyRTC.h"
#include "LightSensor.h"
#include "TempSensor.h"
#include "Buzzer.h"
#include "DataStorage.h"
#include "LED.h"
#include "Encoder.h"
#include "stdio.h"

// 菜单常量定义
#define MENU_DELAY_NO_DATA_MS    2000  // 无数据提示延时(毫秒)
#define MENU_DELAY_ERASE_SEC     3     // 擦除完成延时(秒)
#define MENU_MAIN_ITEM_COUNT     4     // 主菜单项数
#define MENU_HISTORY_ITEM_COUNT  4     // 历史菜单项数
#define MENU_SETTING_ITEM_COUNT  7     // 设置菜单项数
#define MENU_FUNCTION_ITEM_COUNT 3     // 功能菜单项数
#define ALARM_BEEP_DURATION      5     // 报警蜂鸣持续时间(秒)
#define INVALID_MARKER           0xFF  // 未初始化标记值

// 传感器范围限制
#define LIGHT_SENSOR_MAX     999    // 光照传感器最大值
#define TEMP_SENSOR_MAX      115    // 温度传感器最大值 (°C)
#define TEMP_SENSOR_MIN      (-40)  // 温度传感器最小值 (°C)

// 时间相关常量
#define TIME_SECONDS_PER_MINUTE  60  // 每分钟秒数
#define TIME_MINUTES_PER_HOUR    60  // 每小时分钟数
#define TIME_HOURS_PER_DAY       24  // 每天小时数
#define TIME_MAX_HOUR            23  // 最大小时值
#define TIME_MAX_MIN_SEC         59  // 最大分钟/秒值
#define STOPWATCH_MAX_HOUR       99  // 秒表最大小时数

// 调整步长限制
#define STEP_LENGTH_MAX     100  // 单次调整最大步长
#define STEP_LENGTH_MIN     1    // 单次调整最小步长

int Menu1(void);

int Menu2_Stats(void);

int Menu2_History(void);

int Menu2_Setting(void);

int Menu2_Function(void);

int Menu3_Music(void);

int Menu3_SetDate(void);

int Menu3_SetLight(void);

int Menu3_SetTemp(void);

int Menu3_SetLength(void);

int Menu3_AutoSaveSet(void);

int Menu3_ReadRecord(void);

int Menu3_ChipErase(void);

int Menu3_SetMaxRecord(void);

int Menu3_StopWatch(void);

int Menu3_ShowGraph(void);

void DrawStorageChart(uint8_t chartType, int16_t offset, uint8_t totalRecords, uint8_t fullRedraw);

int Menu4_Erase(void);

int Menu4_ToggleSave(void);

int Menu4_SaveInterval(void);

void LCD_ShowRecord(uint8_t RecordID);
#endif
