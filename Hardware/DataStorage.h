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
} DateTime_New;

// 传感器记录结构体（紧凑布局，1字节对齐）
#pragma pack(1)
typedef struct
{
    uint8_t Index;                            // 记录序号（1~250）
    uint16_t Light;                           // 光照值（0~999 lux）
    int16_t Temp;                             // 温度值（实际值×10，如255→25.5℃）
    uint16_t Year;                            // 年份
    uint8_t Month, Day, Hour, Minute, Second; // 日期时间（紧凑定义）
} SensorData_New;
#pragma pack()

// 存储配置常量
#define STORAGE_DATA_START_ADDR 0x000000       // 数据区起始地址（第0扇区开始）
#define STORAGE_CONFIG_ADDR 0x0FF000           // 配置地址（最后一个扇区，用于保存MaxRecordCount）
#define RECORD_SIZE_NEW sizeof(SensorData_New) // 单条记录大小（13字节）
#define MAX_RECORD_COUNT 250                   // 最大记录数（固定250条）
#define SECTOR_SIZE 0x1000                     // 扇区大小（4KB）
#define CONFIG_MAGIC 0xA5C3                    // 配置有效性魔术数字

// 函数声明
void DataStorage_New_Init(void);
uint8_t DataStorage_New_Save(float temp, uint16_t light, const DateTime_New *time);
uint8_t DataStorage_New_Read(uint8_t index, SensorData_New *data);
uint8_t DataStorage_New_GetCount(void);
uint8_t DataStorage_New_GetMaxCount(void);
uint8_t DataStorage_New_SetMaxCount(uint8_t maxCount);
void DataStorage_New_EraseAll(void);
uint8_t DataStorage_New_IsFull(void);

#endif
