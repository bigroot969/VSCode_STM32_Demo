#ifndef __BUZZER_H
#define __BUZZER_H

#include "buzzerdata.h"

extern uint8_t Buzzer_PauseFlag;
extern uint8_t Buzzer_FinishFlag;
extern uint8_t Buzzer_Progress;

void Buzzer_Init(void);
void Buzzer_ON(void);
void Buzzer_OFF(void);
void Buzzer_Sound(uint8_t Note, uint16_t Time);
void Buzzer_Play(const uint8_t *Music);

#endif
