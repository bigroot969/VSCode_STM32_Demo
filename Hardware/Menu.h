#ifndef __MENU_H
#define __MENU_H
#include "stm32f10x.h" // Device header

int Menu1(void);

int Menu2_Stats(void);

int Menu2_History(void);

int Menu2_Setting(void);

int Menu2_Function(void);

int Menu3_Music(void);

int Menu3_SetDate(void);

int Menu3_SetLight(void);

int Menu3_SetTemp(void);

int Menu3_SetLength(void);

int Menu3_AutoSaveSet(void);

int Menu3_ReadRecord(void);

int Menu3_ChipErase(void);

int Menu3_SetMaxRecord(void);

int Menu3_StopWatch(void);

int Menu4_Erase(void);

int Menu4_ToggleSave(void);

int Menu4_SaveInterval(void);

void LCD_ShowRecord(uint8_t RecordID);
#endif
