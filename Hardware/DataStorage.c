#include "DataStorage.h"
#include <string.h>

static uint8_t g_RecordCount=0;  // 当前记录总数
uint8_t MaxRecordCount=10;

/**
 * 初始化存储系统：读取已有记录数并预擦除所有扇区
 */
void DataStorage_Init(void)
{
	SensorData temp;
	W25Q64_Init();  // 初始化W25Q64
	DataStorage_ReadMaxRecord();
	
	// 计算需要的扇区数
	uint32_t totalBytes = MaxRecordCount * RECORD_SIZE;
	uint32_t sectorsNeeded = (totalBytes + 4095) / 4096;
	
	// 读取第一条记录判断是否为新设备
	W25Q64_ReadData(STORAGE_START_ADDR, (uint8_t*)&temp, RECORD_SIZE);
	if (temp.Index == 0xFF)  // 新设备（首条记录未初始化）
	{
			g_RecordCount = 0;
			// 擦除所有需要的扇区
			for (uint32_t i = 0; i < sectorsNeeded; i++)
			{
				W25Q64_SectorErase(STORAGE_START_ADDR + i * 0x1000);
				W25Q64_WaitBusy();
			}
			return;
	}

	// 扫描已有有效记录（最多MAX_RECORD_COUNT条）
	for (g_RecordCount = 0; g_RecordCount < MaxRecordCount; g_RecordCount++)
	{
			uint32_t addr = STORAGE_START_ADDR + g_RecordCount * RECORD_SIZE;
			W25Q64_ReadData(addr, (uint8_t*)&temp, RECORD_SIZE);
			// 检查记录是否有效：Index必须等于期望值且不为0xFF
			if (temp.Index != (g_RecordCount + 1) || temp.Index == 0xFF)
					break;
	}

	// 限制最大记录数（冗余保护）
	if (g_RecordCount > MaxRecordCount){g_RecordCount = 0;}
	
	// 擦除当前记录之后到最大记录范围内的所有空间
	// 计算需要擦除的字节数（从当前记录末尾到MaxRecordCount）
	uint32_t startEraseAddr = STORAGE_START_ADDR + g_RecordCount * RECORD_SIZE;
	uint32_t endEraseAddr = STORAGE_START_ADDR + MaxRecordCount * RECORD_SIZE;
	
	// 按字节擦除未使用空间（通过写入0xFF）
	// 注意：Flash擦除是扇区级别（4KB），这里采用扇区擦除策略
	uint32_t startSector = startEraseAddr / 0x1000;
	uint32_t endSector = (endEraseAddr + 0xFFF) / 0x1000;  // 向上取整
	
	// 如果起始扇区与记录数据共享，需要备份并恢复
	uint32_t currentSector = (STORAGE_START_ADDR + (g_RecordCount > 0 ? (g_RecordCount - 1) : 0) * RECORD_SIZE) / 0x1000;
	
	for (uint32_t sector = startSector; sector < endSector && sector < sectorsNeeded; sector++)
	{
		// 如果当前扇区包含已有数据，跳过或仅擦除未使用部分
		if (sector == currentSector && g_RecordCount > 0)
		{
			// 该扇区包含有效记录，跳过不擦除（让Save函数处理）
			continue;
		}
		W25Q64_SectorErase(STORAGE_START_ADDR + sector * 0x1000);
		W25Q64_WaitBusy();
	}
}


/**
 * 存储一条新记录
 * @return 0：成功；1：失败（存储满/参数无效/写入失败）
 */
uint8_t DataStorage_Save(float temp, uint16_t light, const DateTime* time)
{
    // 检查存储满或时间指针无效
    if (g_RecordCount >= MaxRecordCount || !time)
        return 1;

    // 计算存储地址
    uint32_t addr = STORAGE_START_ADDR + g_RecordCount * RECORD_SIZE;
    
    // 检查是否需要擦除新扇区（仅当跨越扇区边界时）
    uint32_t currentSector = addr / 0x1000;
    uint32_t prevSector = (g_RecordCount > 0) ? ((STORAGE_START_ADDR + (g_RecordCount - 1) * RECORD_SIZE) / 0x1000) : 0xFFFFFFFF;
    
    if (g_RecordCount == 0 || currentSector != prevSector)
    {
        // 跨越到新扇区，检查是否需要擦除
        uint8_t sectorFirstByte;
        W25Q64_ReadData(STORAGE_START_ADDR + currentSector * 0x1000, &sectorFirstByte, 1);
        if (sectorFirstByte != 0xFF)
        {
            // 擦除新扇区
            W25Q64_SectorErase(STORAGE_START_ADDR + currentSector * 0x1000);
            W25Q64_WaitBusy();
        }
    }

    // 填充记录数据
    SensorData data = {
        .Index = g_RecordCount + 1,
        .Temp = (int16_t)(temp * 10),               // 温度放大10倍
        .Light = (light > 999) ? 999 : light,       // 光照限制范围
        .Year = time->Year,
        .Month = time->Month,
        .Day = time->Day,
        .Hour = time->Hour,
        .Minute = time->Minute,
        .Second = time->Second
    };
    
    // 写入新记录
    W25Q64_PageProgram(addr, (uint8_t*)&data, RECORD_SIZE);
    W25Q64_WaitBusy();
    
    // 验证写入
    SensorData verify;
    W25Q64_ReadData(addr, (uint8_t*)&verify, RECORD_SIZE);
    if (memcmp(&data, &verify, RECORD_SIZE) != 0)
        return 1;

    g_RecordCount++;  // 更新记录数
    return 0;
}


/**
 * 按索引读取记录
 * @return 0：成功；1：失败（索引无效/数据异常）
 */
uint8_t DataStorage_Read(uint8_t index, SensorData* data)
{
    if (index >= g_RecordCount || !data)  // 检查索引和指针有效性
        return 1;

    // 读取记录
    uint32_t addr = STORAGE_START_ADDR + index * RECORD_SIZE;
    W25Q64_ReadData(addr, (uint8_t*)data, RECORD_SIZE);
    
    // 验证记录有效性：Index必须匹配且不为0xFF
    if (data->Index != (index + 1) || data->Index == 0xFF)
        return 1;
    
    return 0;
}


// 获取当前记录总数
uint8_t DataStorage_GetCount(void)
{
    return g_RecordCount;
}


// 重置记录计数（仅内存，不同步Flash）
void DataStorage_ResetCount(void)
{
    g_RecordCount = 0;
}

void DataStorage_ReadMaxRecord(void)
{
	uint8_t SavedMaxRecord;
	// 从独立扇区读取2字节（uint16_t类型）
	W25Q64_ReadData(STORAGE_MAX_RECORD_ADDR,(uint8_t*)&SavedMaxRecord,sizeof(uint8_t));
	if (SavedMaxRecord>=1&&SavedMaxRecord<=251)
	{
			MaxRecordCount=SavedMaxRecord;  // 恢复复位前的设置
	}
	else
	{
			MaxRecordCount=10;  // 无效值时用默认值
			DataStorage_SetMaxRecord(MaxRecordCount);  // 写入默认值到Flash
	}
}

uint8_t DataStorage_SetMaxRecord(uint8_t MaxRecord)
{
	if(MaxRecord<1||MaxRecord>250){return 1;}
	if (MaxRecord<g_RecordCount)
	{
		for (uint16_t i = MaxRecord; i < g_RecordCount; i++)
		{
			uint32_t addr = STORAGE_START_ADDR + i * RECORD_SIZE;
			SensorData invalid = {.Index = 0xFF};  // 仅标记Index为无效（0xFF）
			// 只写入Index字段（1字节），避免覆盖其他数据
			W25Q64_PageProgram(addr, (uint8_t*)&invalid.Index, sizeof(invalid.Index));
			W25Q64_WaitBusy();
		}
		g_RecordCount = MaxRecord;  // 更新当前记录数为新的最大值
	}
	uint32_t sectorAddr=STORAGE_MAX_RECORD_ADDR&0xFFFFF000;
	W25Q64_SectorErase(sectorAddr);
	W25Q64_PageProgram(STORAGE_MAX_RECORD_ADDR, (uint8_t*)&MaxRecord, sizeof(uint8_t));
	
	uint8_t verify;
	W25Q64_ReadData(STORAGE_MAX_RECORD_ADDR, (uint8_t*)&verify, sizeof(uint8_t));
	if (verify != MaxRecord)
			return 1;  // 写入失败
	
	MaxRecordCount = MaxRecord;  // 更新全局变量
	return 0;  // 成功
}
