#ifndef __LCD_H
#define __LCD_H
#include "MySPI.h"
#include "LCD_Data.h"
// -------------------------- 屏幕参数宏定义 --------------------------
#define LCD_WIDTH 128
#define LCD_HEIGHT 160
// 显示方向宏定义
#define LCD_DIR_PORTRAIT_0 0xC0    // 纵向，左上角（0，0）- 0度
#define LCD_DIR_LANDSCAPE_90 0xA0  // 右转90° 横向
#define LCD_DIR_PORTRAIT_180 0x00  // 右转180° 纵向
#define LCD_DIR_LANDSCAPE_270 0x60 // 右转270° 横向

// 颜色
#define RED 0xf800
#define GREEN 0x07e0
#define BLUE 0x001f
#define BLUE2 0x1c9f
#define PINK 0xd8a7
#define ORANGE 0xfa20
#define WHITE 0xffff
#define BLACK 0x0000
#define YELLOW 0xFFE0
#define CYAN 0x07ff
#define PURPLE 0xf81f
#define PURPLE2 0xdb92
#define PURPLE3 0x8811
#define GRAY0 0xEF7D
#define GRAY1 0x8410
#define GRAY2 0x4208

/*FontSize参数取值*/
/*此参数值不仅用于判断，而且用于计算横向字符偏移，默认值为字体像素宽度*/
#define LCD_8X16 8
#define LCD_6X8 6

/* 汉字字体大小 */
#define LCD_16X16 16 // 16x16像素汉字

void LCD_Init(void);                                                                                                                 // LCD 初始化
void LCD_SendCMD(uint8_t CMD);                                                                                                       // 发送命令
void LCD_SendData(uint8_t Data);                                                                                                     // 发送单字节数据
void LCD_SendData16(uint16_t Data);                                                                                                  // 发送 16 位颜色数据
void LCD_SetAddrWindow(uint16_t X0, uint16_t Y0, uint16_t X1, uint16_t Y1);                                                          // 设置显示窗口
void LCD_Fill(uint16_t Color);                                                                                                       // 全屏填充
void LCD_DrawPoint(uint16_t X, uint16_t Y, uint16_t Color);                                                                          // 画点
void LCD_DrawLine(uint16_t X1, uint16_t Y1, uint16_t X2, uint16_t Y2, uint16_t Color);                                               // 画线
void LCD_SetDirection(uint8_t Locate);                                                                                               // 设置显示方向
void LCD_Clear(uint16_t Color);                                                                                                      // 清屏
void LCD_DrawRectangle(uint16_t X1, uint16_t Y1, uint16_t X2, uint16_t Y2, uint16_t Color);                                          // 绘制矩形边框
void LCD_DrawRectangle_Fill(uint16_t X1, uint16_t Y1, uint16_t X2, uint16_t Y2, uint16_t Color);                                     // 绘制填充矩形
void LCD_ShowImage(uint16_t X, uint16_t Y, uint16_t Width, uint16_t Height, const uint8_t *Image, uint16_t FColor, uint16_t BColor); // 显示图像
void LCD_ShowChar(uint16_t X, uint16_t Y, uint8_t Ch, uint8_t FontSize, uint16_t FColor, uint16_t BColor);                                      // 显示单字符
void LCD_ShowString(uint16_t X, uint16_t Y, const char *Str, uint8_t FontSize, uint16_t FColor, uint16_t BColor, uint8_t Center);           // 显示字符串（Center=1 居中）
void LCD_ShowNum(uint16_t X, uint16_t Y, uint32_t Num, uint8_t FontSize, uint16_t FColor, uint16_t BColor, uint8_t Center, uint8_t LeadingZero, uint8_t Digits);  // 显示无符号整数（Center=1 居中，LeadingZero=1 前导零，Digits 显示位数）
void LCD_ShowSignedNum(uint16_t X, uint16_t Y, int32_t Num, uint8_t FontSize, uint16_t FColor, uint16_t BColor, uint8_t Center, uint8_t LeadingZero, uint8_t Digits);            // 显示有符号整数（Center=1 居中，LeadingZero=1 前导零，Digits 显示位数）
void LCD_ShowHexNum(uint16_t X, uint16_t Y, uint32_t Num, uint8_t FontSize, uint16_t FColor, uint16_t BColor, uint8_t Center, uint8_t LeadingZero, uint8_t Digits);              // 显示十六进制数（Center=1 居中，LeadingZero=1 前导零，Digits 显示位数不含0x）
void LCD_ShowBinNum(uint16_t X, uint16_t Y, uint32_t Num, uint8_t FontSize, uint16_t FColor, uint16_t BColor, uint8_t Center, uint8_t LeadingZero, uint8_t Digits);              // 显示二进制数（Center=1 居中，LeadingZero=1 前导零，Digits 显示位数不含0b）
void LCD_ShowFloatNum(uint16_t X, uint16_t Y, double Num, uint8_t Decimals, uint8_t FontSize, uint16_t FColor, uint16_t BColor, uint8_t Center, uint8_t FixedWidth); // 显示浮点数（Center=1 居中，FixedWidth=固定字符宽度，0=自动）
void LCD_ShowChinese(uint16_t X, uint16_t Y, const char *Chinese, uint8_t FontSize, uint16_t FColor, uint16_t BColor, uint8_t Center);      // 显示汉字（Center=1 居中）

#endif
