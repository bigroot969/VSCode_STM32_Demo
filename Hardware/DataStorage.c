#include "DataStorage.h"
#include <string.h>
#include "IWDG.h"

// 内部全局变量（简化设计：只在RAM中维护）
static uint8_t g_MaxRecordCount = MAX_RECORD_COUNT;    // 最大记录数（默认最大值）
static uint8_t g_CurrentRecordCount = 0; // 当前已存储记录数（通过扫描得出）

/**
 * @brief  初始化存储系统（简化版）
 * @note   直接扫描Flash，确定当前记录数
 */
void DataStorage_New_Init(void)
{
    SensorData_New temp;
    uint16_t configData[2]; // [0]=Magic, [1]=MaxRecordCount

    // 初始化W25Q64
    W25Q64_Init();

    // 读取配置
    W25Q64_ReadData(STORAGE_CONFIG_ADDR, (uint8_t *)configData, 4);

    // 检查配置有效性
    if (configData[0] == CONFIG_MAGIC && configData[1] >= 1 && configData[1] <= MAX_RECORD_COUNT)
    {
        g_MaxRecordCount = (uint8_t)configData[1]; // 恢复用户设置
    }
    else
    {
        g_MaxRecordCount = 250; // 使用默认值
    }

    g_CurrentRecordCount = 0;

    // 检查第一条记录位置，如果Index不是1且不是0xFF，说明Flash未正确初始化
    W25Q64_ReadData(STORAGE_DATA_START_ADDR, (uint8_t *)&temp, RECORD_SIZE_NEW);
    if (temp.Index != 1 && temp.Index != 0xFF)
    {
        // Flash数据异常，全部擦除
        DataStorage_New_EraseAll();
        return;
    }

    // 扫描Flash，确定当前实际存储的记录数
    uint8_t indexBuf;
    for (uint8_t i = 0; i < g_MaxRecordCount; i++)
    {
        uint32_t addr = STORAGE_DATA_START_ADDR + i * RECORD_SIZE_NEW;
        // 优化：只读取第一个字节(Index)来判断是否为空，减少SPI通信量
        W25Q64_ReadData(addr, &indexBuf, 1);

        // 检查是否为空位置（全FF或Index不匹配）
        if (indexBuf == 0xFF || indexBuf != (i + 1))
        {
            break; // 遇到第一个空位置，停止扫描
        }

        g_CurrentRecordCount++;
    }
} /**
   * @brief  保存一条新记录（简化版）
   * @param  temp: 温度值
   * @param  light: 光照值
   * @param  time: 时间结构体指针
   * @retval 0-成功，1-失败（存储满/参数无效/写入失败）
   */
uint8_t DataStorage_New_Save(float temp, uint16_t light, const DateTime_New *time)
{
    // 参数检查
    if (!time)
    {
        return 1;
    }

    // 检查是否已满
    if (g_CurrentRecordCount >= g_MaxRecordCount)
    {
        return 1; // 存储已满
    }

    // 计算存储地址
    uint32_t addr = STORAGE_DATA_START_ADDR + g_CurrentRecordCount * RECORD_SIZE_NEW;
    uint32_t endAddr = addr + RECORD_SIZE_NEW - 1;

    // 计算涉及的扇区
    uint32_t startSector = (addr / SECTOR_SIZE) * SECTOR_SIZE;
    uint32_t endSector = (endAddr / SECTOR_SIZE) * SECTOR_SIZE;

    // 获取上一条记录结束位置所在的扇区
    uint32_t prevEndSector = 0xFFFFFFFF;
    if (g_CurrentRecordCount > 0)
    {
        uint32_t prevAddr = STORAGE_DATA_START_ADDR + (g_CurrentRecordCount - 1) * RECORD_SIZE_NEW;
        uint32_t prevEndAddr = prevAddr + RECORD_SIZE_NEW - 1;
        prevEndSector = (prevEndAddr / SECTOR_SIZE) * SECTOR_SIZE;
    }

    // 1. 检查起始扇区：如果与上一条记录结束所在的扇区不同，说明进入了新扇区，需要擦除
    if (startSector != prevEndSector)
    {
        W25Q64_SectorErase(startSector);
        W25Q64_WaitBusy();
    }

    // 2. 检查结束扇区：如果记录跨越了扇区边界，且结束扇区与起始扇区不同，说明尾部进入了下一个新扇区，也需要擦除
    if (endSector != startSector)
    {
        W25Q64_SectorErase(endSector);
        W25Q64_WaitBusy();
    }

    // 填充数据
    SensorData_New data;
    data.Index = g_CurrentRecordCount + 1;
    data.Temp = (int16_t)(temp * 10);
    data.Light = (light > 999) ? 999 : light;
    data.Year = time->Year;
    data.Month = time->Month;
    data.Day = time->Day;
    data.Hour = time->Hour;
    data.Minute = time->Minute;
    data.Second = time->Second;

    // 写入Flash（处理跨页边界问题）
    uint32_t pageAddr = (addr / 256) * 256; // 当前页起始地址
    uint32_t pageEnd = pageAddr + 256;      // 当前页结束地址

    if (addr + RECORD_SIZE_NEW <= pageEnd)
    {
        // 不跨页，直接写入
        W25Q64_PageProgram(addr, (uint8_t *)&data, RECORD_SIZE_NEW);
        W25Q64_WaitBusy();
    }
    else
    {
        // 跨页，分两次写入
        uint8_t firstPartSize = pageEnd - addr; // 第一页剩余空间
        uint8_t secondPartSize = RECORD_SIZE_NEW - firstPartSize;

        // 写入第一部分
        W25Q64_PageProgram(addr, (uint8_t *)&data, firstPartSize);
        W25Q64_WaitBusy();

        // 写入第二部分
        W25Q64_PageProgram(pageEnd, (uint8_t *)&data + firstPartSize, secondPartSize);
        W25Q64_WaitBusy();
    }

    // 读回验证
    SensorData_New verify;
    W25Q64_ReadData(addr, (uint8_t *)&verify, RECORD_SIZE_NEW);

    // 验证Index和关键数据
    if (verify.Index != data.Index)
    {
        return 1; // 写入失败
    }

    // 成功，更新计数
    g_CurrentRecordCount++;

    return 0;
}

/**
 * @brief  按索引读取记录
 * @param  index: 记录索引（0-based，0表示第1条）
 * @param  data: 数据结构体指针
 * @retval 0-成功，1-失败
 */
uint8_t DataStorage_New_Read(uint8_t index, SensorData_New *data)
{
    if (!data || index >= g_CurrentRecordCount)
    {
        return 1;
    }

    uint32_t addr = STORAGE_DATA_START_ADDR + index * RECORD_SIZE_NEW;
    W25Q64_ReadData(addr, (uint8_t *)data, RECORD_SIZE_NEW);

    // 简单验证：Index应该匹配
    if (data->Index != (index + 1))
    {
        return 1;
    }

    return 0;
}

/**
 * @brief  获取当前记录数
 */
uint8_t DataStorage_New_GetCount(void)
{
    return g_CurrentRecordCount;
}

/**
 * @brief  获取最大记录数（固定250）
 */
uint8_t DataStorage_New_GetMaxCount(void)
{
    return g_MaxRecordCount;
}

/**
 * @brief  设置最大记录数（简化：仅更新内存变量）
 * @param  maxCount: 最大记录数（1-250）
 * @retval 0-成功，1-失败
 */
uint8_t DataStorage_New_SetMaxCount(uint8_t maxCount)
{
    if (maxCount < 1 || maxCount > MAX_RECORD_COUNT)
    {
        return 1;
    }

    // 如果减小最大值且当前记录超出，截断计数
    if (maxCount < g_CurrentRecordCount)
    {
        g_CurrentRecordCount = maxCount;
    }

    g_MaxRecordCount = maxCount;

    // 保存配置到Flash
    uint16_t configData[2];
    configData[0] = CONFIG_MAGIC;
    configData[1] = maxCount;

    // 读取旧配置，检查是否需要更新
    uint16_t oldConfig[2];
    W25Q64_ReadData(STORAGE_CONFIG_ADDR, (uint8_t *)oldConfig, 4);

    // 如果配置不同，需要擦除后写入
    if (oldConfig[0] != configData[0] || oldConfig[1] != configData[1])
    {
        W25Q64_SectorErase(STORAGE_CONFIG_ADDR);
        W25Q64_WaitBusy();
        W25Q64_PageProgram(STORAGE_CONFIG_ADDR, (uint8_t *)configData, 4);
        W25Q64_WaitBusy();
    }

    return 0;
}

/**
 * @brief  擦除所有数据
 */
void DataStorage_New_EraseAll(void)
{
    // 计算需要擦除的扇区数
    uint32_t sectorsNeeded = (MAX_RECORD_COUNT * RECORD_SIZE_NEW + SECTOR_SIZE - 1) / SECTOR_SIZE;

    // 擦除所有数据扇区
    for (uint32_t i = 0; i < sectorsNeeded; i++)
    {
        W25Q64_SectorErase(STORAGE_DATA_START_ADDR + i * SECTOR_SIZE);
        W25Q64_WaitBusy();
        IWDG_Feed(); // 喂狗，防止长时间擦除导致复位
    }

    // 重置计数（保持用户设置的最大记录数不变）
    g_CurrentRecordCount = 0;
    // 注意：不重置 g_MaxRecordCount，保留用户设置
}

/**
 * @brief  检查存储是否已满
 */
uint8_t DataStorage_New_IsFull(void)
{
    return (g_CurrentRecordCount >= g_MaxRecordCount) ? 1 : 0;
}
