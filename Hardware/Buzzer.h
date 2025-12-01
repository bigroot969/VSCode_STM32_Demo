#ifndef __BUZZER_H
#define __BUZZER_H

#include "BuzzerData.h"

extern uint8_t Buzzer_PauseFlag;
extern uint8_t Buzzer_FinishFlag;
extern uint8_t Buzzer_Progress;
extern uint16_t Buzzer_Speed;
void Buzzer_Init(void);
void Buzzer_Tick(void);
uint16_t BPM2Speed(uint16_t BPM);
void Buzzer_ON(void);
void Buzzer_OFF(void);
void Buzzer_Sound(uint8_t Note, uint16_t Time);
void Buzzer_Play(const uint8_t *Music);

#endif
