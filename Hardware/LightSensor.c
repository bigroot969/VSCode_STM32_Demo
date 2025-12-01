#include "stm32f10x.h"
#include "LightSensor.h"
#include "Delay.h"
// 数字
void LIGHTSENSOR_DInit(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}
uint8_t LIGHTSENSOR_DGET(void)
{
	return GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_3);
}
// 模拟
void LDR_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(LDR_GPIO_CLK, ENABLE); // 打开 ADC IO端口时钟
	GPIO_InitStructure.GPIO_Pin = LDR_GPIO_PIN;	  // 配置 ADC IO 引脚模式
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN; // 设置为模拟输入

	GPIO_Init(LDR_GPIO_PORT, &GPIO_InitStructure); // 初始化 ADC IO

	ADCx_Init();
}

uint16_t LDR_ADC_Read(void)
{
	// 设置指定ADC的规则组通道，采样时间
	return ADC_GetValue(ADC_CHANNEL, ADC_SampleTime_55Cycles5);
}

uint16_t LDR_Average_Data(void)
{
	uint32_t tempData = 0;
	for (uint8_t i = 0; i < LDR_READ_TIMES; i++)
	{
		tempData += LDR_ADC_Read();
		// Delay_ms(5); // 移除延时以提高响应速度
	}

	tempData /= LDR_READ_TIMES;
	return (uint16_t)tempData;
}

uint16_t LDR_LuxData()
{
	float voltage = 0;
	float R = 0;
	uint16_t Lux = 0;
	voltage = LDR_Average_Data();
	voltage = voltage / 4096 * 3.3f;

	R = voltage / (3.3f - voltage) * 10000;

	Lux = 40000 * pow(R, -0.6021);

	if (Lux > 999)
	{
		Lux = 999;
	}
	return Lux;
}
