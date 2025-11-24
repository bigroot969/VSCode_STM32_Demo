#ifndef __DATASTORAGE_H
#define __DATASTORAGE_H

#include "stm32f10x.h"
#include "W25Q64.h"
// 日期时间结构体（与RTC模块适配）
typedef struct
{
    uint16_t Year;  // 年份（如2025）
    uint8_t Month;  // 月份（1-12）
    uint8_t Day;    // 日期（1-31）
    uint8_t Hour;   // 小时（0-23）
    uint8_t Minute; // 分钟（0-59）
    uint8_t Second; // 秒（0-59）
} DateTime;

// 传感器记录结构体（紧凑布局，1字节对齐）
#pragma pack(1)
typedef struct
{
    uint8_t Index;                            // 记录序号（1~255）
    uint16_t Light;                           // 光照值（0~999 lux）
    int16_t Temp;                             // 温度值（实际值×10，如255→25.5℃）
    uint16_t Year;                            // 年份
    uint8_t Month, Day, Hour, Minute, Second; // 日期时间（紧凑定义）
} SensorData;
#pragma pack()

// 存储配置
#define STORAGE_START_ADDR 0x000000 // 存储起始地址（数据记录区）
#define STORAGE_MAX_RECORD_ADDR 0x002000 // MaxRecordCount配置地址（第3个扇区，更安全）
#define RECORD_SIZE sizeof(SensorData) // 单条记录大小（自动计算）
extern uint8_t MaxRecordCount;
// 函数声明
void DataStorage_Init(void);
uint8_t DataStorage_Save(float temp, uint16_t light, const DateTime *time);
uint8_t DataStorage_Read(uint8_t index, SensorData *data);
uint8_t DataStorage_GetCount(void);
void DataStorage_ResetCount(void);
void DataStorage_ReadMaxRecord(void);
uint8_t DataStorage_SetMaxRecord(uint8_t MaxRecord);
#endif
