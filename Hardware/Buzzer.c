#include "stm32f10x.h" // Device header
#include "Delay.h"
#include "PWM.h"
#include "Buzzer.h"

/*引脚配置*/
#define Buzzer_IO GPIO_Pin_0 // PB0

// BPM = 120, 16分音符时长，计算公式：60000 ms / BPM / 4 = 125 ms，注：BPM为每分钟拍数（4分音符个数）
// #define Buzzer_Speed        125
uint16_t Buzzer_Speed = 125;
uint8_t Buzzer_Progress;   // 播放进度
uint8_t Buzzer_PauseFlag;  // 停止标志
uint8_t Buzzer_FinishFlag; // 结束标志

// 新增状态变量
static uint8_t Buzzer_State = 0;	  // 0:空闲, 1:播放中, 2:间歇
static uint16_t Buzzer_TimeCount = 0; // 倒计时

/**
 * @brief  蜂鸣器初始化函数，初始化PB0为蜂鸣器输出
 * @param  无
 * @retval 无
 */
void Buzzer_Init(void)
{
	PWM_Init();

	Buzzer_PauseFlag = 1;
	Buzzer_FinishFlag = 0;
	Buzzer_Progress = 0;
	Buzzer_State = 0;
	Buzzer_TimeCount = 0;
}

/**
 * @brief  蜂鸣器时基函数，在定时器中断中调用（1ms）
 * @param  无
 * @retval 无
 */
void Buzzer_Tick(void)
{
	if (Buzzer_TimeCount > 0)
	{
		Buzzer_TimeCount--;
	}
}

uint16_t BPM2Speed(uint16_t BPM)
{
	if (BPM == 0)
	{
		return 125;
	} // 避免除零，默认125ms
	else
	{
		return 60000 / BPM / 4;
	}
}

/**
 * @brief  蜂鸣器鸣叫函数，频率：2kHz
 * @param  无
 * @retval 无
 */
void Buzzer_ON(void)
{
	PWM_SetPrescaler(360 - 1);
	TIM_Cmd(TIM3, ENABLE);
}

/**
 * @brief  蜂鸣器停止鸣叫函数
 * @param  无
 * @retval 无
 */
void Buzzer_OFF(void)
{
	TIM_Cmd(TIM3, DISABLE);
	Buzzer_State = 0;
}

/**
 * @brief  蜂鸣器发声函数
 * @param  Note 指定发声的音符，范围：L1 ~ H7
 * @param  Time 指定发声的时间，范围：0 ~ 65535，单位：ms
 * @retval 无
 */
void Buzzer_Sound(uint8_t Note, uint16_t Time)
{
	if (Note) // 非休止符
	{
		PWM_SetPrescaler(Buzzer_Freq[Note]);
		TIM_Cmd(TIM3, ENABLE);
		Delay_ms(Time);
		TIM_Cmd(TIM3, DISABLE);
		Delay_ms(5);
	}
	else // 休止符
	{
		Delay_ms(Time);
		Delay_ms(5);
	}
}

/**
 * @brief  蜂鸣器播放函数，在main的主函数中循环调用（非阻塞模式）
 * @param  Music 指定要播放的音乐
 * @retval 无
 */
void Buzzer_Play(const uint8_t *Music)
{
	static uint16_t Music_Index = 0;
	uint8_t NoteSelect, TimeSelect;
	uint16_t duration;

	if (Buzzer_Progress == 0)
	{
		Music_Index = 0;
		Buzzer_FinishFlag = 0;
		Buzzer_State = 0;
		Buzzer_TimeCount = 0;
	}

	if (!Buzzer_FinishFlag && !Buzzer_PauseFlag)
	{
		if (Buzzer_TimeCount == 0)
		{
			if (Buzzer_State == 1) // 刚刚播放完一个音符，进入间歇
			{
				TIM_Cmd(TIM3, DISABLE);
				Buzzer_TimeCount = 5; // 间歇5ms
				Buzzer_State = 2;
			}
			else // 刚刚完成间歇（或初始状态），播放下一个音符
			{
				NoteSelect = Music[2 * (Music_Index + 1)];
				if (NoteSelect == 0xFF)
				{
					Buzzer_FinishFlag = 1;
					Buzzer_PauseFlag = 1;
					Buzzer_State = 0;
					TIM_Cmd(TIM3, DISABLE);
					return;
				}

				TimeSelect = Music[2 * (Music_Index + 1) + 1];
				duration = TimeSelect * Buzzer_Speed;

				if (NoteSelect)
				{
					PWM_SetPrescaler(Buzzer_Freq[NoteSelect]);
					TIM_Cmd(TIM3, ENABLE);
				}
				else
				{
					TIM_Cmd(TIM3, DISABLE);
				}

				Buzzer_TimeCount = duration;
				Buzzer_State = 1;

				Music_Index++;
				Buzzer_Progress = 1 + Music_Index * 120 / (Music[0] * 256 + Music[1]);
			}
		}
	}
}
