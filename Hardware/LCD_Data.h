#ifndef __LCD_DATA_H
#define __LCD_DATA_H
#include <stdint.h>

/*字符集定义*/
/*以下两个宏定义只可解除其中一个的注释*/
// #define LCD_CHARSET_UTF8        // 定义字符集为UTF8
#define LCD_CHARSET_GB2312 // 定义字符集为GB2312

/*中文字符字节宽度*/
#ifdef LCD_CHARSET_UTF8
#define LCD_CHN_CHAR_WIDTH 3 // UTF-8编码格式结3字节
#endif
#ifdef LCD_CHARSET_GB2312
#define LCD_CHN_CHAR_WIDTH 2 // GB2312编码格式结2字节
#endif

/*字模基本单元*/
typedef struct
{
#ifdef LCD_CHARSET_UTF8 // 定义字符集为UTF8
    char Index[5];      // 汉字索引，空间为5字节
#endif

#ifdef LCD_CHARSET_GB2312 // 定义字符集为GB2312
    char Index[3];        // 汉字索引，空间为3字节
#endif

    uint8_t Data[32]; // 字模数据
} LCD_ChineseCell_t;

// 字模数据（ASCII 32..127）
// LCD_F6X8: 6x8 字体，每个字符占 6 字节（每字节表示一列的 8 像素）
// LCD_F8X16: 8x16 字体，每个字符占 16 字节（每字节表示该行的 8 像素，共 16 行）

extern const uint8_t LCD_F6X8[][6];
extern const uint8_t LCD_F8X16[][16];
/*汉字字模数据声明*/
extern const LCD_ChineseCell_t LCD_CF16x16[];

#endif
