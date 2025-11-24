#ifndef __BUZZER_DATA_H
#define __BUZZER_DATA_H

#include <stdint.h>

/*音符定义*/
#define P 0    // 休止符
#define L1 1   // 低音1
#define L1_ 2  // 低音1#
#define L2 3   // 低音2
#define L2_ 4  // 低音2#
#define L3 5   // 低音3
#define L4 6   // 低音4
#define L4_ 7  // 低音4#
#define L5 8   // 低音5
#define L5_ 9  // 低音5#
#define L6 10  // 低音6
#define L6_ 11 // 低音6#
#define L7 12  // 低音7
#define M1 13  // 中音1
#define M1_ 14 // 中音1#
#define M2 15  // 中音2
#define M2_ 16 // 中音2#
#define M3 17  // 中音3
#define M4 18  // 中音4
#define M4_ 19 // 中音4#
#define M5 20  // 中音5
#define M5_ 21 // 中音5#
#define M6 22  // 中音6
#define M6_ 23 // 中音6#
#define M7 24  // 中音7
#define H1 25  // 高音1
#define H1_ 26 // 高音1#
#define H2 27  // 高音2
#define H2_ 28 // 高音2#
#define H3 29  // 高音3
#define H4 30  // 高音4
#define H4_ 31 // 高音4#
#define H5 32  // 高音5
#define H5_ 33 // 高音5#
#define H6 34  // 高音6
#define H6_ 35 // 高音6#
#define H7 36  // 高音7

/*频率数据声明*/
extern const uint16_t Buzzer_Freq[];
extern const uint16_t MusicBPM[];
/*音乐数据声明*/
extern const uint8_t LittleStar[];
extern const uint8_t CastleInTheSky[];
extern const uint8_t Haruhikage[];
extern const uint8_t Orb[];
extern const uint8_t Ori[];
/*按照上面的格式，在这个位置加入新的音乐数据声明*/
//...

#endif
