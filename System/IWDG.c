#include "IWDG.h"

/**
 * @brief  初始化独立看门狗 (IWDG)
 * @param  timeout_ms: 超时时间(毫秒)
 * @retval None
 * @note   IWDG时钟: LSI = 40kHz
 *         计算公式: Tout = (4 * 2^PRER * RLR) / 40
 *         其中 PRER = 预分频值 (0~7), RLR = 重装载值 (0~0xFFF)
 */
void IWDG_Init(uint16_t timeout_ms)
{
	uint8_t prescaler;
	uint16_t reload;

	// 根据超时时间选择合适的预分频值和重装载值
	// LSI = 40kHz, 预分频后频率 = 40000 / (4 * 2^PRER) Hz
	// 超时时间(ms) = (reload * 1000) / 频率

	if (timeout_ms <= 512)
	{
		// PRER = 2: 分频16, 频率 = 2500Hz, 最大超时 = 4095/2500*1000 = 1638ms
		prescaler = IWDG_Prescaler_16; // 16分频
		reload = (timeout_ms * 2500) / 1000;
	}
	else if (timeout_ms <= 1024)
	{
		// PRER = 3: 分频32, 频率 = 1250Hz, 最大超时 = 4095/1250*1000 = 3276ms
		prescaler = IWDG_Prescaler_32; // 32分频
		reload = (timeout_ms * 1250) / 1000;
	}
	else if (timeout_ms <= 2048)
	{
		// PRER = 4: 分频64, 频率 = 625Hz, 最大超时 = 4095/625*1000 = 6552ms
		prescaler = IWDG_Prescaler_64; // 64分频
		reload = (timeout_ms * 625) / 1000;
	}
	else if (timeout_ms <= 4096)
	{
		// PRER = 5: 分频128, 频率 = 312.5Hz, 最大超时 = 4095/312.5*1000 = 13104ms
		prescaler = IWDG_Prescaler_128; // 128分频
		reload = (uint16_t)((timeout_ms * 3125UL) / 10000UL);
	}
	else
	{
		// PRER = 6: 分频256, 频率 = 156.25Hz, 最大超时 = 4095/156.25*1000 = 26208ms
		prescaler = IWDG_Prescaler_256; // 256分频
		reload = (uint16_t)((timeout_ms * 15625UL) / 100000UL);
	}

	// 限制重装载值范围
	if (reload > 0xFFF)
		reload = 0xFFF; // 最大4095
	if (reload < 1)
		reload = 1; // 最小1

	// 配置看门狗
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable); // 使能写访问
	IWDG_SetPrescaler(prescaler);				   // 设置预分频值
	IWDG_SetReload(reload);						   // 设置重装载值
	IWDG_ReloadCounter();						   // 重载计数器(喂狗)
	IWDG_Enable();								   // 启动看门狗
}

/**
 * @brief  喂狗 (重载看门狗计数器)
 * @param  None
 * @retval None
 */
void IWDG_Feed(void)
{
	IWDG_ReloadCounter();
}
