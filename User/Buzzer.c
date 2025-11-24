#include "stm32f10x.h"                  // Device header
#include "delay.h"
#include "pwm.h"
#include "buzzer.h"

/*引脚配置*/
#define Buzzer_IO        GPIO_Pin_0    //PB0

//Buzzer_Speed=60000 ms / BPM / 4 ，BPM为每分钟拍数（4分音符个数）
#define Buzzer_Speed        172
uint8_t Buzzer_StartFlag;
uint8_t Buzzer_Progress;       //播放进度
uint8_t Buzzer_PauseFlag;      //停止标志
uint8_t Buzzer_FinishFlag;     //结束标志

/**
  * @brief  蜂鸣器初始化函数，初始化PB0为蜂鸣器输出
  * @param  无
  * @retval 无
  */
void Buzzer_Init(void)
{
	PWM_Init();
	Buzzer_StartFlag=1;
	/*Buzzer_PauseFlag = 1;
	Buzzer_FinishFlag = 0;
	Buzzer_Progress = 0;*/
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
}

/**
  * @brief  蜂鸣器发声函数
  * @param  Note 指定发声的音符，范围：L1 ~ H7
  * @param  Time 指定发声的时间，范围：0 ~ 65535，单位：ms
  * @retval 无
  */
void Buzzer_Sound(uint8_t Note, uint16_t Time)
{
	if(Note)    //非休止符
	{
		PWM_SetPrescaler(Buzzer_Freq[Note]);
		TIM_Cmd(TIM3, ENABLE);
		Delay_ms(Time);
		TIM_Cmd(TIM3, DISABLE);
		Delay_ms(5);
	}
	else    //休止符
	{
		Delay_ms(Time);
		Delay_ms(5);
	}
}

/**
  * @brief  蜂鸣器播放函数，在main的主函数中循环调用
  * @param  Music 指定要播放的音乐
  * @retval 无
  */
void Buzzer_Play(const uint8_t *Music)
{
	static uint16_t i = 0;
	uint8_t NoteSelect, TimeSelect;
	
	//if(!Buzzer_Progress)  {i = 0; Buzzer_FinishFlag = 0;}    //Buzzer_Progress清零时重置计数器
	
	//if(!Buzzer_FinishFlag && !Buzzer_PauseFlag)    //暂停或结束时不执行
	{                                              
		NoteSelect = Music[2 * (i + 1)];           //读取音符
		if(NoteSelect == 0xFF)                     //若读取到结束符0xFF，则退出并置标志位
		{ 
			Buzzer_StartFlag=0;                                                   
			Buzzer_FinishFlag = 1;                           
			Buzzer_PauseFlag = 1;                            
			return;                                          
		}                                                    
		TimeSelect = Music[2 * (i + 1) + 1];                            //读取时长（16分音符时长的几倍）
		Buzzer_Sound(NoteSelect, TimeSelect * Buzzer_Speed);            //蜂鸣器发声
		i++;
		//Buzzer_Progress = 1 + i * 120 / (Music[0] * 256 + Music[1]);    //计算进度，+1防止进度不增加时i直接清零
	}
	
}
