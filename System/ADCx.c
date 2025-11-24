#include "stm32f10x.h" // Device header
#include "ADCx.h"
/**
 * 函    数：AD初始化
 * 参    数：无
 * 返 回 值：无
 */
void ADCx_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(ADC_CLK, ENABLE);

	/*设置ADC时钟*/
	RCC_ADCCLKConfig(RCC_PCLK2_Div6); // 选择时钟6分频，ADCCLK = 72MHz / 6 = 12MHz

	ADC_InitTypeDef ADC_InitStructure;
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;					// 独立模式
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;				// 数据右对齐
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None; // 软件触发
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;					// 单次转换
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;						// 非扫描模式
	ADC_InitStructure.ADC_NbrOfChannel = 1;								// 总通道数
	ADC_Init(ADCx, &ADC_InitStructure);									// 初始化ADC1

	/*ADC使能*/
	ADC_Cmd(ADCx, ENABLE); // 使能ADC1，ADC开始运行
	// 进行ADC校准
	ADC_ResetCalibration(ADCx);
	while (ADC_GetResetCalibrationStatus(ADCx) == SET)
		;
	ADC_StartCalibration(ADCx);
	while (ADC_GetCalibrationStatus(ADCx) == SET)
		;
}

/**
 * 函    数：获取AD转换的值
 * 参    数：无
 * 返 回 值：AD转换的值，范围：0~4095
 */
uint16_t ADC_GetValue(uint8_t ADC_Channel, uint8_t ADC_SampleTime)
{
	ADC_RegularChannelConfig(ADCx, ADC_Channel, 1, ADC_SampleTime);

	ADC_SoftwareStartConvCmd(ADCx, ENABLE); // 软件触发ADC转换
	while (ADC_GetFlagStatus(ADCx, ADC_FLAG_EOC) == RESET)
		; // 读取ADC转换完成标志位
	return ADC_GetConversionValue(ADCx);
}
