#include "LCD.h"
#include "Delay.h"
#include <stddef.h>
#include <string.h>

// 寄存器操作宏优化，替代库函数调用以提升速度
#define LCD_CS_CLR() LCD_CS_PORT->BRR = LCD_CS_PIN
#define LCD_CS_SET() LCD_CS_PORT->BSRR = LCD_CS_PIN
#define LCD_RS_CLR() LCD_RS_PORT->BRR = LCD_RS_PIN
#define LCD_RS_SET() LCD_RS_PORT->BSRR = LCD_RS_PIN

// 内联辅助函数：等待 SPI 发送缓冲区空
static inline void LCD_WaitTXE(void)
{
    while (SPI_I2S_GetFlagStatus(LCD_SPI, SPI_I2S_FLAG_TXE) == RESET)
        ;
}

// 内联辅助函数：等待 SPI 传输完成
static inline void LCD_WaitBSY(void)
{
    while (SPI_I2S_GetFlagStatus(LCD_SPI, SPI_I2S_FLAG_BSY) == SET)
        ;
}

// 内联辅助函数：发送单字节（不含等待 BSY）
static inline void LCD_SendByte(uint8_t byte)
{
    LCD_WaitTXE();
    SPI_I2S_SendData(LCD_SPI, byte);
}

void LCD_SendCMD(uint8_t CMD)
{
    LCD_CS_CLR(); // 选中屏幕
    LCD_RS_CLR(); // RS=0（命令）

    LCD_SendByte(CMD);
    LCD_WaitBSY();

    LCD_CS_SET(); // 取消选中
}

void LCD_SendData(uint8_t Data)
{
    LCD_CS_CLR(); // 选中屏幕
    LCD_RS_SET(); // RS=1（数据）

    LCD_SendByte(Data);
    LCD_WaitBSY();

    LCD_CS_SET(); // 取消选中
}

void LCD_SendData16(uint16_t Data)
{
    LCD_CS_CLR(); // 选中屏幕
    LCD_RS_SET(); // RS=1（数据）

    // 先发送高 8 位，再发送低 8 位
    LCD_SendByte(Data >> 8);
    LCD_SendByte(Data & 0xFF);
    LCD_WaitBSY();

    LCD_CS_SET(); // 取消选中
}

void LCD_Reset(void)
{
    GPIO_ResetBits(LCD_RST_PORT, LCD_RST_PIN); // RST=0
    Delay_ms(100);
    GPIO_SetBits(LCD_RST_PORT, LCD_RST_PIN); // RST=1
    Delay_ms(100);
}

void LCD_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    // 使能 GPIOA 和 SPI1 时钟（均挂载 APB2）
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | LCD_SPI_CLK, ENABLE);

    // 1. 配置 CS/RS/RST 为推挽输出
    GPIO_InitStruct.GPIO_Pin = LCD_CS_PIN | LCD_RS_PIN | LCD_RST_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LCD_CS_PORT, &GPIO_InitStruct);

    // 2. 配置 SPI1 引脚（SCK/PA5、MOSI/PA7）为复用推挽输出
    GPIO_InitStruct.GPIO_Pin = LCD_SCK_PIN | LCD_MOSI_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP; // 复用推挽
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 初始状态：CS 拉高（未选中）、RST 拉高（正常工作）
    GPIO_SetBits(LCD_CS_PORT, LCD_CS_PIN);
    GPIO_SetBits(LCD_RST_PORT, LCD_RST_PIN);
}

void LCD_Init(void)
{
    LCD_GPIO_Init();
    MySPI1_Init();
    LCD_Reset();

    // ---------------------- 初始化序列----------------------
    LCD_SendCMD(0x11); // 【唤醒显示屏】从睡眠模式恢复
    Delay_ms(120);     // 必要延时

    // ---------------------- 基本配置 ----------------------
    LCD_SendCMD(0x36);  // 【数据访问方式】RGB/BGR、行列方向等
    LCD_SendData(0x00); // 临时配置（后续会重新设置）

    LCD_SendCMD(0x3A);  // 【RGB图像数据格式】
    LCD_SendData(0x05); // 16bit RGB565 格式

    LCD_SendCMD(0xB1); // 帧率控制（正常模式/全彩）
    LCD_SendData(0x05);
    LCD_SendData(0x3C);
    LCD_SendData(0x3C);

    LCD_SendCMD(0xB2); // 帧率控制（空闲模式/8色）
    LCD_SendData(0x05);
    LCD_SendData(0x3C);
    LCD_SendData(0x3C);

    LCD_SendCMD(0xB3); // 帧率控制（部分模式/全彩）
    LCD_SendData(0x05);
    LCD_SendData(0x3C);
    LCD_SendData(0x3C);
    LCD_SendData(0x05);
    LCD_SendData(0x3C);
    LCD_SendData(0x3C);

    LCD_SendCMD(0xB4); // 显示反转控制
    LCD_SendData(0x03);

    // ---------------------- 电源控制寄存器1-5 ----------------------
    LCD_SendCMD(0xC0); // 特定颜色模式电压参数
    LCD_SendData(0x2E);
    LCD_SendData(0x06);
    LCD_SendData(0x04);

    LCD_SendCMD(0xC1); // 精细电压调整
    LCD_SendData(0xC0);
    LCD_SendData(0xC2);

    LCD_SendCMD(0xC2); // 电压调整
    LCD_SendData(0x0D);
    LCD_SendData(0x0D);

    LCD_SendCMD(0xC3); // 电压调整
    LCD_SendData(0x8D);
    LCD_SendData(0xEE);

    LCD_SendCMD(0xC4); // 电压调整
    LCD_SendData(0x8D);
    LCD_SendData(0xEE);

    LCD_SendCMD(0xC5); // 设置VCOM电压
    LCD_SendData(0x00);

    // ---------------------- 数据显示方式（重要：设置显示方向）----------------------
    LCD_SendCMD(0x36);
    LCD_SendData(0xC0); // 默认0°纵向（可通过LCD_SpinScreen函数修改）

    // ---------------------- 伽马序列 ----------------------
    LCD_SendCMD(0xE0); // 正电压伽马校正
    LCD_SendData(0x1B);
    LCD_SendData(0x21);
    LCD_SendData(0x10);
    LCD_SendData(0x15);
    LCD_SendData(0x2B);
    LCD_SendData(0x25);
    LCD_SendData(0x1F);
    LCD_SendData(0x23);
    LCD_SendData(0x22);
    LCD_SendData(0x22);
    LCD_SendData(0x2B);
    LCD_SendData(0x37);
    LCD_SendData(0x00);
    LCD_SendData(0x15);
    LCD_SendData(0x02);
    LCD_SendData(0x3F);

    LCD_SendCMD(0xE1); // 负电压伽马校正
    LCD_SendData(0x1A);
    LCD_SendData(0x20);
    LCD_SendData(0x0F);
    LCD_SendData(0x15);
    LCD_SendData(0x2A);
    LCD_SendData(0x25);
    LCD_SendData(0x1E);
    LCD_SendData(0x23);
    LCD_SendData(0x23);
    LCD_SendData(0x22);
    LCD_SendData(0x2B);
    LCD_SendData(0x37);
    LCD_SendData(0x00);
    LCD_SendData(0x15);
    LCD_SendData(0x02);
    LCD_SendData(0x3F);

    // ---------------------- 自定补充操作 ----------------------
    LCD_SendCMD(0x2C);   // 初始化显示参数配置
    LCD_SendCMD(0x29);   // 【打开屏幕】（0x28为关闭）
    LCD_SetDirection(2); // 纵向显示
    LCD_Clear(WHITE);    // 初始清屏为黑色
}

void LCD_SetAddrWindow(uint16_t X0, uint16_t Y0, uint16_t X1, uint16_t Y1)
{
    LCD_SendCMD(0x2A); // 列地址设置
    LCD_SendData16(X0);
    LCD_SendData16(X1);
    LCD_SendCMD(0x2B); // 行地址设置
    LCD_SendData16(Y0);
    LCD_SendData16(Y1);

    LCD_SendCMD(0x2C); // 准备写入颜色数据
}

void LCD_Fill(uint16_t Color)
{
    uint8_t color_h = Color >> 8;
    uint8_t color_l = Color & 0xFF;

    LCD_SetAddrWindow(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1); // 全屏窗口

    LCD_CS_CLR();
    LCD_RS_SET();

    // 连续发送 128*160 个颜色数据
    // 循环展开优化，减少循环判断开销
    uint32_t total_pixels = LCD_WIDTH * LCD_HEIGHT;
    while (total_pixels >= 8)
    {
        LCD_SendByte(color_h);
        LCD_SendByte(color_l);
        LCD_SendByte(color_h);
        LCD_SendByte(color_l);
        LCD_SendByte(color_h);
        LCD_SendByte(color_l);
        LCD_SendByte(color_h);
        LCD_SendByte(color_l);
        LCD_SendByte(color_h);
        LCD_SendByte(color_l);
        LCD_SendByte(color_h);
        LCD_SendByte(color_l);
        LCD_SendByte(color_h);
        LCD_SendByte(color_l);
        LCD_SendByte(color_h);
        LCD_SendByte(color_l);
        total_pixels -= 8;
    }
    while (total_pixels--)
    {
        LCD_SendByte(color_h);
        LCD_SendByte(color_l);
    }
    LCD_WaitBSY();

    LCD_CS_SET();
}

// 画点（x,y 坐标，color 颜色）
void LCD_DrawPoint(uint16_t X, uint16_t Y, uint16_t Color)
{
    if (X >= LCD_WIDTH || Y >= LCD_HEIGHT)
        return;                    // 坐标越界判断
    LCD_SetAddrWindow(X, Y, X, Y); // 单个像素窗口
    LCD_SendData16(Color);         // 写入颜色
}

// 清屏（封装全屏填充）
void LCD_Clear(uint16_t Color)
{
    LCD_Fill(Color);
}

void LCD_SetDirection(uint8_t Locate)
{
    LCD_SendCMD(0x36); // 屏幕的显示方向、像素读写顺序

    if (Locate == 0)
        LCD_SendData(LCD_DIR_PORTRAIT_0); // 纵向，左上角（0，0）
    else if (Locate == 1)
        LCD_SendData(LCD_DIR_LANDSCAPE_90); // 右转90° 横向
    else if (Locate == 2)
        LCD_SendData(LCD_DIR_PORTRAIT_180); // 右转180° 纵向
    else if (Locate == 3)
        LCD_SendData(LCD_DIR_LANDSCAPE_270); // 右转270° 横向

    Delay_ms(10); // 等待设置生效
}

// 画线函数（使用Bresenham算法）
/**
 * @brief 绘制一条从 (X1, Y1) 到 (X2, Y2) 的直线
 *
 * @param X1 起点X坐标
 * @param Y1 起点Y坐标
 * @param X2 终点X坐标
 * @param Y2 终点Y坐标
 * @param Color 线条颜色
 */
void LCD_DrawLine(uint16_t X1, uint16_t Y1, uint16_t X2, uint16_t Y2, uint16_t Color)
{
    int16_t dx, dy, sx, sy, err, e2;

    // 计算x和y方向的增量
    dx = (X2 > X1) ? (X2 - X1) : (X1 - X2);
    dy = (Y2 > Y1) ? (Y2 - Y1) : (Y1 - Y2);

    // 确定步进方向
    sx = (X1 < X2) ? 1 : -1;
    sy = (Y1 < Y2) ? 1 : -1;

    // 初始误差项
    err = dx - dy;

    // 绘制线段
    while (1)
    {
        // 绘制当前点
        LCD_DrawPoint(X1, Y1, Color);

        // 判断是否到达终点
        if ((X1 == X2) && (Y1 == Y2))
            break;

        // 计算下一个点的位置
        e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            X1 += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            Y1 += sy;
        }
    }
}

// 绘制矩形边框
/**
 * @brief 绘制一个矩形边框，从 (X1, Y1) 到 (X2, Y2)
 *
 * @param X1 矩形左上角X坐标
 * @param Y1 矩形左上角Y坐标
 * @param X2 矩形右下角X坐标
 * @param Y2 矩形右下角Y坐标
 * @param Color 矩形边框颜色
 */
void LCD_DrawRectangle(uint16_t X1, uint16_t Y1, uint16_t X2, uint16_t Y2, uint16_t Color)
{
    // 如果x1=x2或y1=y2，不绘制任何图形
    if (X1 == X2 || Y1 == Y2 || X2 < X1 || Y2 < Y1)
        return;
    // 确保坐标在有效范围内
    if (X1 >= LCD_WIDTH)
        X1 = LCD_WIDTH - 1;
    if (X2 >= LCD_WIDTH)
        X2 = LCD_WIDTH - 1;
    if (Y1 >= LCD_HEIGHT)
        Y1 = LCD_HEIGHT - 1;
    if (Y2 >= LCD_HEIGHT)
        Y2 = LCD_HEIGHT - 1;

    // 确保x1 <= x2和y1 <= y2
    if (X1 > X2)
    {
        uint16_t temp = X1;
        X1 = X2;
        X2 = temp;
    }
    if (Y1 > Y2)
    {
        uint16_t temp = Y1;
        Y1 = Y2;
        Y2 = temp;
    }

    // 绘制四条边
    LCD_DrawLine(X1, Y1, X2, Y1, Color); // 上边
    LCD_DrawLine(X1, Y2, X2, Y2, Color); // 下边
    LCD_DrawLine(X1, Y1, X1, Y2, Color); // 左边
    LCD_DrawLine(X2, Y1, X2, Y2, Color); // 右边
}

// 绘制填充矩形
/**
 * @brief 绘制一个填充矩形，从 (X1, Y1) 到 (X2, Y2)
 *
 * @param X1 矩形左上角X坐标
 * @param Y1 矩形左上角Y坐标
 * @param X2 矩形右下角X坐标
 * @param Y2 矩形右下角Y坐标
 * @param Color 矩形填充颜色
 */
void LCD_DrawRectangle_Fill(uint16_t X1, uint16_t Y1, uint16_t X2, uint16_t Y2, uint16_t Color)
{
    uint32_t pixel_count;
    uint8_t color_h = Color >> 8;
    uint8_t color_l = Color & 0xFF;

    // 如果x1=x2或y1=y2，不绘制任何图形
    if (X1 == X2 || Y1 == Y2 || X2 < X1 || Y2 < Y1)
        return;

    // 确保坐标在有效范围内
    if (X1 >= LCD_WIDTH)
        X1 = LCD_WIDTH - 1;
    if (X2 >= LCD_WIDTH)
        X2 = LCD_WIDTH - 1;
    if (Y1 >= LCD_HEIGHT)
        Y1 = LCD_HEIGHT - 1;
    if (Y2 >= LCD_HEIGHT)
        Y2 = LCD_HEIGHT - 1;

    // 确保x1 <= x2和y1 <= y2
    if (X1 > X2)
    {
        uint16_t temp = X1;
        X1 = X2;
        X2 = temp;
    }
    if (Y1 > Y2)
    {
        uint16_t temp = Y1;
        Y1 = Y2;
        Y2 = temp;
    }

    // 设置矩形区域窗口
    LCD_SetAddrWindow(X1, Y1, X2, Y2);

    // 填充矩形区域
    LCD_CS_CLR();
    LCD_RS_SET();

    pixel_count = (uint32_t)(X2 - X1 + 1) * (Y2 - Y1 + 1);

    // 循环展开优化
    while (pixel_count >= 8)
    {
        LCD_SendByte(color_h);
        LCD_SendByte(color_l);
        LCD_SendByte(color_h);
        LCD_SendByte(color_l);
        LCD_SendByte(color_h);
        LCD_SendByte(color_l);
        LCD_SendByte(color_h);
        LCD_SendByte(color_l);
        LCD_SendByte(color_h);
        LCD_SendByte(color_l);
        LCD_SendByte(color_h);
        LCD_SendByte(color_l);
        LCD_SendByte(color_h);
        LCD_SendByte(color_l);
        LCD_SendByte(color_h);
        LCD_SendByte(color_l);
        pixel_count -= 8;
    }
    while (pixel_count--)
    {
        LCD_SendByte(color_h);
        LCD_SendByte(color_l);
    }
    LCD_WaitBSY();

    LCD_CS_SET();
}

// 显示单个 ASCII 字符
/**
 * @brief 显示单个 ASCII 字符
 *
 * @param X 字符左上角X坐标
 * @param Y 字符左上角Y坐标
 * @param Ch ASCII 字符
 * @param FontSize 字体大小（LCD_6X8 或 LCD_8X16）
 * @param FColor 字体颜色
 * @param BColor 背景颜色
 */
void LCD_ShowChar(uint16_t X, uint16_t Y, uint8_t Ch, uint8_t FontSize, uint16_t FColor, uint16_t BColor)
{
    if (Ch < ' ' || Ch > 0x7F)
        return; // 非打印 ASCII

    uint8_t cindex = Ch - ' ';

    if (FontSize == LCD_6X8)
    {
        /* 6x8 字体是列优先格式（每字节代表一列 8 个像素），需要转换为行优先格式 */
        uint8_t bitmap_6x8[8]; // 8 行，每行 1 字节

        for (uint8_t row = 0; row < 8; row++)
        {
            bitmap_6x8[row] = 0;
            for (uint8_t col = 0; col < 6; col++)
            {
                uint8_t col_byte = LCD_F6X8[cindex][col];
                if (col_byte & (1 << row))
                    bitmap_6x8[row] |= (1 << col); // LSB->MSB: 从左到右映射到bit0到bit5
            }
        }
        LCD_ShowImage(X, Y, 6, 8, bitmap_6x8, FColor, BColor);
    }
    else if (FontSize == LCD_8X16)
    {
        /* 8x16 字体已是行优先格式，直接使用 */
        LCD_ShowImage(X, Y, 8, 16, LCD_F8X16[cindex], FColor, BColor);
    }
}

// 显示字符串
/**
 * @brief 显示字符串
 *
 * @param X 字符串起始左上角X坐标（Center=1 时忽略）
 * @param Y 字符串起始左上角Y坐标
 * @param Str 字符串指针
 * @param FontSize 字体大小（LCD_6X8 或 LCD_8X16）
 * @param FColor 字体颜色
 * @param BgColor 背景颜色
 * @param Center 1=水平居中显示，0=从 X,Y 开始显示
 */
void LCD_ShowString(uint16_t X, uint16_t Y, const char *Str, uint8_t FontSize, uint16_t FColor, uint16_t BgColor, uint8_t Center)
{
    if (Str == NULL)
        return;

    // 居中显示模式
    if (Center)
    {
        uint16_t total_width = 0;
        const char *p = Str;

        // 统计像素宽度
        while (*p != '\0' && *p != '\n')
        {
            if ((uint8_t)*p < 0x80)
            {
                total_width += (FontSize == LCD_6X8) ? 6 : 8;
                p++;
            }
            else
            {
                total_width += LCD_16X16;
                p++;
                while (*p != '\0' && ((uint8_t)*p & 0xC0) == 0x80)
                    p++;
            }
        }

        if (LCD_WIDTH > total_width)
            X = (LCD_WIDTH - total_width) / 2;
        else
            X = 0;
    }

    // 显示字符串
    uint16_t x0 = X;
    uint8_t char_width = (FontSize == LCD_6X8) ? 6 : 8;
    uint8_t char_height = (FontSize == LCD_6X8) ? 8 : 16;

    while (*Str != '\0')
    {
        if (*Str == '\n')
        {
            Y += char_height;
            X = x0;
            if (Y >= LCD_HEIGHT)
                break;
        }
        else if (*Str == '\r')
        {
            X = x0;
        }
        else
        {
            if (X < LCD_WIDTH && Y < LCD_HEIGHT)
            {
                LCD_ShowChar(X, Y, *Str, FontSize, FColor, BgColor);
            }
            X += char_width;
            if (X >= LCD_WIDTH)
            {
                Y += char_height;
                X = x0;
                if (Y >= LCD_HEIGHT)
                    break;
            }
        }
        Str++;
    }
}

/**
 * @brief 显示图像
 *
 * @param X 图像左上角X坐标
 * @param Y 图像左上角Y坐标
 * @param Width 图像宽度（像素）
 * @param Height 图像高度（像素）
 * @param Image 指向图像字模数据的指针（行优先，每行按字节存储像素位）
 * @param FColor 前景色（像素为 1 时的颜色）
 * @param BColor 背景色（像素为 0 时的颜色）
 */
void LCD_ShowImage(uint16_t X, uint16_t Y, uint16_t Width, uint16_t Height, const uint8_t *Image, uint16_t FColor, uint16_t BColor)
{
    if (X >= LCD_WIDTH || Y >= LCD_HEIGHT || Image == NULL)
        return;

    uint16_t x_end = (X + Width - 1 >= LCD_WIDTH) ? (LCD_WIDTH - 1) : (X + Width - 1);
    uint16_t y_end = (Y + Height - 1 >= LCD_HEIGHT) ? (LCD_HEIGHT - 1) : (Y + Height - 1);
    uint8_t fcolor_h = FColor >> 8;
    uint8_t fcolor_l = FColor & 0xFF;
    uint8_t bcolor_h = BColor >> 8;
    uint8_t bcolor_l = BColor & 0xFF;

    LCD_SetAddrWindow(X, Y, x_end, y_end);
    GPIO_ResetBits(LCD_CS_PORT, LCD_CS_PIN);
    GPIO_SetBits(LCD_RS_PORT, LCD_RS_PIN);

    uint16_t bytes_per_row = (Width + 7) / 8;

    for (uint16_t row = 0; row < Height; row++)
    {
        for (uint16_t col = 0; col < Width; col++)
        {
            uint16_t byte_idx = col / 8;
            uint8_t bit_pos = col % 8; // LSB->MSB: 从bit0到bit7
            uint8_t byte_data = Image[row * bytes_per_row + byte_idx];

            if (byte_data & (1 << bit_pos))
            {
                LCD_SendByte(fcolor_h);
                LCD_SendByte(fcolor_l);
            }
            else
            {
                LCD_SendByte(bcolor_h);
                LCD_SendByte(bcolor_l);
            }
        }
    }
    LCD_WaitBSY();

    GPIO_SetBits(LCD_CS_PORT, LCD_CS_PIN);
}

// 显示无符号整数（10进制）
/**
 * @brief 显示无符号整数（10进制）
 * @param X 整数起始左上角X坐标
 * @param Y 整数起始左上角Y坐标
 * @param Num 要显示的无符号整数
 * @param FontSize 字体大小（LCD_6X8 或 LCD_8X16）
 * @param FColor 字体颜色
 * @param BColor 背景颜色
 * @param Center 1=水平居中显示，0=从 X,Y 开始显示
 * @param LeadingZero 1=前导零填充，0=不填充
 * @param Digits 指定显示的位数（0 表示不限制）
 */
void LCD_ShowNum(uint16_t X, uint16_t Y, uint32_t Num, uint8_t FontSize, uint16_t FColor, uint16_t BColor, uint8_t Center, uint8_t LeadingZero, uint8_t Digits)
{
    char buffer[11];
    uint8_t i = 0;

    // 如果指定了显示位数，使用格式化输出
    if (Digits > 0 && Digits <= 10)
    {
        char temp[11];
        uint8_t j = 0;
        uint32_t num_copy = Num;

        // 提取数字的各位
        if (num_copy == 0)
        {
            temp[j++] = '0';
        }
        else
        {
            while (num_copy > 0 && j < Digits)
            {
                temp[j++] = '0' + (num_copy % 10);
                num_copy /= 10;
            }
        }

        // 如果需要前导零，补齐位数
        if (LeadingZero)
        {
            while (j < Digits)
            {
                temp[j++] = '0';
            }
        }

        // 反转到buffer中
        for (i = 0; i < j; i++)
        {
            buffer[i] = temp[j - 1 - i];
        }
        buffer[j] = '\0';

        LCD_ShowString(X, Y, buffer, FontSize, FColor, BColor, Center);
        return;
    }

    // 原有逻辑：不指定位数时的处理
    if (Num == 0)
    {
        LCD_ShowString(X, Y, "0", FontSize, FColor, BColor, Center);
        return;
    }

    char temp[11];
    uint8_t j = 0;
    while (Num > 0)
    {
        temp[j++] = '0' + (Num % 10);
        Num /= 10;
    }

    for (i = 0; i < j; i++)
    {
        buffer[i] = temp[j - 1 - i];
    }
    buffer[j] = '\0';

    LCD_ShowString(X, Y, buffer, FontSize, FColor, BColor, Center);
}

// 内部辅助函数：水平居中显示字符串（用于汉字居中）
static void LCD_ShowString_Center_Internal(uint16_t Y, const char *Str, uint8_t FontSize, uint16_t FColor, uint16_t BColor)
{
    if (Str == NULL)
        return;

    uint16_t total_width = 0;
    const char *p = Str;

    // 统计像素宽度（简单：把每个字符看作固定宽度，6 或 8）
    while (*p != '\0' && *p != '\n')
    {
        if ((uint8_t)*p < 0x80)
        {
            total_width += (FontSize == LCD_6X8) ? 6 : 8;
            p++;
        }
        else
        {
            // 如果遇到非 ASCII，按一个汉字 16 像素宽处理并跳过 UTF-8 的后续字节（通常 2 个或更多）
            total_width += LCD_16X16;
            // 跳过 UTF-8 连续字节（0x80..0xBF）
            p++; // 跳过首字节
            while (*p != '\0' && ((uint8_t)*p & 0xC0) == 0x80)
                p++;
        }
    }

    uint16_t x = 0;
    if (LCD_WIDTH > total_width)
        x = (LCD_WIDTH - total_width) / 2;

    // 利用现有显示函数（对于 ASCII 部分，LCD_ShowString 可直接处理；对汉字它会忽略非 ASCII）
    // 为兼容混合字符串，逐段输出：当遇到 ASCII 连续区间用 LCD_ShowString 输出，当遇到汉字块用 LCD_ShowChinese 输出
    uint16_t curX = x;
    p = Str;
    while (*p != '\0' && *p != '\n')
    {
        // 如果是 ASCII 字节，收集连续 ASCII 子串
        if ((uint8_t)*p < 0x80)
        {
            const char *start_p = p;
            while (*p != '\0' && *p != '\n' && ((uint8_t)*p < 0x80))
                p++;
            // 显示这一段 ASCII
            size_t len = p - start_p;
            if (len > 0)
            {
                // 创建一个临时以 NUL 结尾的子串
                char tmp[128];
                if (len >= sizeof(tmp))
                    len = sizeof(tmp) - 1;
                memcpy(tmp, start_p, len);
                tmp[len] = '\0';
                LCD_ShowString(curX, Y, tmp, FontSize, FColor, BColor, 0);
                curX += (uint16_t)len * ((FontSize == LCD_6X8) ? 6 : 8);
            }
        }
        else
        {

            // 计算 UTF-8 codepoint length by leading byte
            uint8_t lead = (uint8_t)*p;
            uint8_t bytes = 1;
            if ((lead & 0xE0) == 0xC0)
                bytes = 2;
            else if ((lead & 0xF0) == 0xE0)
                bytes = 3;
            else if ((lead & 0xF8) == 0xF0)
                bytes = 4;

            // Display single chinese char using LCD_ShowChinese
            // prepare a small buffer containing this single UTF-8 char
            char chbuf[5] = {0};
            for (uint8_t k = 0; k < bytes && *p != '\0'; k++)
            {
                chbuf[k] = *p;
                p++;
            }
            chbuf[bytes] = '\0';
            LCD_ShowChinese(curX, Y, chbuf, LCD_16X16, FColor, BColor, 0);
            curX += LCD_16X16;
        }
    }
}

// 内部辅助函数：水平居中显示汉字
static void LCD_ShowChinese_Center_Internal(uint16_t Y, const char *Chinese, uint8_t FontSize, uint16_t FColor, uint16_t BColor)
{
    if (Chinese == NULL)
        return;

    // 仅支持 16x16 字体
    if (FontSize != LCD_16X16)
    {
        // 若非 16x16，则退回到普通字符串居中处理
        LCD_ShowString_Center_Internal(Y, Chinese, FontSize, FColor, BColor);
        return;
    }

    // 统计总宽度（支持 UTF-8 和 GB2312 混合）
    const char *p = Chinese;
    uint16_t total_width = 0;

    while (*p != '\0' && *p != '\n')
    {
        uint8_t ch = (uint8_t)*p;

#ifdef LCD_CHARSET_UTF8
        if (ch < 0x80)
        {
            // ASCII 字符，8 像素宽
            total_width += 8;
            p++;
        }
        else if ((ch & 0xE0) == 0xC0)
        {
            // 2 字节 UTF-8 字符（通常不是汉字）
            total_width += 16;
            p += 2;
        }
        else if ((ch & 0xF0) == 0xE0)
        {
            // 3 字节 UTF-8 字符（汉字）
            total_width += 16;
            p += 3;
        }
        else if ((ch & 0xF8) == 0xF0)
        {
            // 4 字节 UTF-8 字符
            total_width += 16;
            p += 4;
        }
        else
        {
            p++;
        }
#endif

#ifdef LCD_CHARSET_GB2312
        if (ch < 0x80)
        {
            // ASCII 字符，8 像素宽
            total_width += 8;
            p++;
        }
        else
        {
            // 2 字节 GB2312 字符（汉字）
            total_width += 16;
            p += 2;
        }
#endif
    }

    uint16_t x = 0;
    if (LCD_WIDTH > total_width)
        x = (LCD_WIDTH - total_width) / 2;

    LCD_ShowChinese(x, Y, Chinese, FontSize, FColor, BColor, 0);
}

// 显示有符号整数（10进制）
/**
 * @brief 显示有符号整数（10进制）
 *
 * @param X 有符号整数起始左上角X坐标
 * @param Y 有符号整数起始左上角Y坐标
 * @param Num 要显示的有符号整数
 * @param FontSize 字体大小（LCD_6X8 或 LCD_8X16）
 * @param FColor 字体颜色
 * @param BColor 背景颜色
 * @param Center 1=水平居中显示，0=从 X,Y 开始显示
 */
void LCD_ShowSignedNum(uint16_t X, uint16_t Y, int32_t Num, uint8_t FontSize, uint16_t FColor, uint16_t BColor, uint8_t Center, uint8_t LeadingZero, uint8_t Digits)
{
    char buffer[13];
    uint8_t i = 0;
    uint8_t isNegative = 0;

    if (Num < 0)
    {
        isNegative = 1;
        Num = -Num;
    }

    // 如果指定了显示位数
    if (Digits > 0 && Digits <= 10)
    {
        char temp[11];
        uint8_t j = 0;
        uint32_t num_copy = Num;

        if (num_copy == 0)
        {
            temp[j++] = '0';
        }
        else
        {
            while (num_copy > 0 && j < Digits)
            {
                temp[j++] = '0' + (num_copy % 10);
                num_copy /= 10;
            }
        }

        // 如果需要前导零，补齐位数
        if (LeadingZero)
        {
            while (j < Digits)
            {
                temp[j++] = '0';
            }
        }

        // 添加负号
        if (isNegative)
        {
            buffer[i++] = '-';
        }

        // 反转数字到buffer
        for (uint8_t k = 0; k < j; k++)
        {
            buffer[i++] = temp[j - 1 - k];
        }
        buffer[i] = '\0';

        LCD_ShowString(X, Y, buffer, FontSize, FColor, BColor, Center);
        return;
    }

    // 原有逻辑：不指定位数时
    char *p = buffer + 12;
    *p = '\0';

    if (Num == 0)
    {
        *(--p) = '0';
    }
    else
    {
        while (Num > 0)
        {
            *(--p) = '0' + (Num % 10);
            Num /= 10;
        }
    }

    if (isNegative)
    {
        *(--p) = '-';
    }

    LCD_ShowString(X, Y, p, FontSize, FColor, BColor, Center);
}

// 显示十六进制数（0x前缀）
/**
 * @brief 显示十六进制数（0x前缀）
 *
 * @param X 十六进制数起始左上角X坐标
 * @param Y 十六进制数起始左上角Y坐标
 * @param Num 要显示的十六进制数
 * @param FontSize 字体大小（LCD_6X8 或 LCD_8X16）
 * @param FColor 字体颜色
 * @param BColor 背景颜色
 * @param Center 1=水平居中显示，0=从 X,Y 开始显示
 */
void LCD_ShowHexNum(uint16_t X, uint16_t Y, uint32_t Num, uint8_t FontSize, uint16_t FColor, uint16_t BColor, uint8_t Center, uint8_t LeadingZero, uint8_t Digits)
{
    char buffer[13];
    char HexTable[] = "0123456789ABCDEF";

    // 如果指定了显示位数
    if (Digits > 0 && Digits <= 8)
    {
        buffer[0] = '0';
        buffer[1] = 'x';
        uint8_t i = 2;

        char temp[9];
        uint8_t j = 0;
        uint32_t num_copy = Num;

        if (num_copy == 0)
        {
            temp[j++] = '0';
        }
        else
        {
            while (num_copy > 0 && j < Digits)
            {
                temp[j++] = HexTable[num_copy & 0x0F];
                num_copy >>= 4;
            }
        }

        // 如果需要前导零，补齐位数
        if (LeadingZero)
        {
            while (j < Digits)
            {
                temp[j++] = '0';
            }
        }

        // 反转到buffer
        for (uint8_t k = 0; k < j; k++)
        {
            buffer[i++] = temp[j - 1 - k];
        }
        buffer[i] = '\0';

        LCD_ShowString(X, Y, buffer, FontSize, FColor, BColor, Center);
        return;
    }

    // 原有逻辑：不指定位数时
    char *p = buffer + 12;
    *p = '\0';

    if (Num == 0)
    {
        *(--p) = '0';
    }
    else
    {
        while (Num > 0)
        {
            *(--p) = HexTable[Num & 0x0F];
            Num >>= 4;
        }
    }

    *(--p) = 'x';
    *(--p) = '0';

    LCD_ShowString(X, Y, p, FontSize, FColor, BColor, Center);
}

// 显示二进制数（0b前缀）
/**
 * @brief 显示二进制数（0b前缀）
 *
 * @param X 二进制数起始左上角X坐标
 * @param Y 二进制数起始左上角Y坐标
 * @param Num 要显示的二进制数
 * @param FontSize 字体大小（LCD_6X8 或 LCD_8X16）
 * @param FColor 字体颜色
 * @param BColor 背景颜色
 * @param Center 1=水平居中显示，0=从 X,Y 开始显示
 */
void LCD_ShowBinNum(uint16_t X, uint16_t Y, uint32_t Num, uint8_t FontSize, uint16_t FColor, uint16_t BColor, uint8_t Center, uint8_t LeadingZero, uint8_t Digits)
{
    char buffer[35];

    // 如果指定了显示位数
    if (Digits > 0 && Digits <= 32)
    {
        buffer[0] = '0';
        buffer[1] = 'b';
        uint8_t i = 2;

        char temp[33];
        uint8_t j = 0;
        uint32_t num_copy = Num;

        if (num_copy == 0)
        {
            temp[j++] = '0';
        }
        else
        {
            while (num_copy > 0 && j < Digits)
            {
                temp[j++] = '0' + (num_copy & 1);
                num_copy >>= 1;
            }
        }

        // 如果需要前导零，补齐位数
        if (LeadingZero)
        {
            while (j < Digits)
            {
                temp[j++] = '0';
            }
        }

        // 反转到buffer
        for (uint8_t k = 0; k < j; k++)
        {
            buffer[i++] = temp[j - 1 - k];
        }
        buffer[i] = '\0';

        LCD_ShowString(X, Y, buffer, FontSize, FColor, BColor, Center);
        return;
    }

    // 原有逻辑：不指定位数时
    char *p = buffer + 34;
    *p = '\0';

    if (Num == 0)
    {
        *(--p) = '0';
    }
    else
    {
        while (Num > 0)
        {
            *(--p) = '0' + (Num & 1);
            Num >>= 1;
        }
    }

    *(--p) = 'b';
    *(--p) = '0';

    LCD_ShowString(X, Y, p, FontSize, FColor, BColor, Center);
}

// 显示浮点数
/**
 * @brief 显示浮点数
 *
 * @param X 浮点数起始左上角X坐标
 * @param Y 浮点数起始左上角Y坐标
 * @param Num 要显示的浮点数
 * @param Decimals 小数点后位数
 * @param FontSize 字体大小（LCD_6X8 或 LCD_8X16）
 * @param FColor 字体颜色
 * @param BColor 背景颜色
 * @param Center 1=水平居中显示，0=从 X,Y 开始显示
 * @param FixedWidth 固定字符宽度（0=自动宽度，>0=指定宽度用背景色填充）
 */
void LCD_ShowFloatNum(uint16_t X, uint16_t Y, double Num, uint8_t Decimals, uint8_t FontSize, uint16_t FColor, uint16_t BColor, uint8_t Center, uint8_t FixedWidth)
{
    char buffer[32];
    char *p = buffer;

    if (Num < 0)
    {
        *p++ = '-';
        Num = -Num;
    }

    // 添加舍入补偿
    double rounding = 0.5;
    for (uint8_t i = 0; i < Decimals; i++)
    {
        rounding /= 10.0;
    }
    Num += rounding;

    uint32_t IntPart = (uint32_t)Num;
    double DecPart = Num - IntPart;

    char IntBuffer[11];
    char *q = IntBuffer + 10;
    *q = '\0';

    if (IntPart == 0)
    {
        *(--q) = '0';
    }
    else
    {
        while (IntPart > 0)
        {
            *(--q) = '0' + (IntPart % 10);
            IntPart /= 10;
        }
    }

    while (*q != '\0')
    {
        *p++ = *q++;
    }

    if (Decimals > 0)
    {
        *p++ = '.';

        for (uint8_t i = 0; i < Decimals; i++)
        {
            DecPart *= 10;
            uint8_t Digit = (uint8_t)DecPart;
            *p++ = '0' + Digit;
            DecPart -= Digit;
        }
    }

    *p = '\0';

    // 如果指定了固定宽度，先清除足够宽的区域
    if (FixedWidth > 0 && !Center)
    {
        uint16_t clearWidth = FixedWidth * FontSize;
        uint16_t clearHeight = (FontSize == LCD_6X8) ? 8 : 16;
        LCD_DrawRectangle_Fill(X, Y, X + clearWidth - 1, Y + clearHeight - 1, BColor);
    }

    LCD_ShowString(X, Y, buffer, FontSize, FColor, BColor, Center);
}

/**
 * @brief 显示汉字
 *
 * @param X 汉字起始左上角X坐标
 * @param Y 汉字起始左上角Y坐标
 * @param Chinese 指向汉字字符串的指针（支持UTF-8和GB2312混合，以'\0'结尾）
 * @param FontSize 汉字字体大小，当前支持 16（16x16像素）
 * @param FColor 字体颜色
 * @param BColor 背景颜色
 * @param Center 1=水平居中显示，0=从 X,Y 开始显示
 */
void LCD_ShowChinese(uint16_t X, uint16_t Y, const char *Chinese, uint8_t FontSize, uint16_t FColor, uint16_t BColor, uint8_t Center)
{
    uint16_t i = 0;
    char SingleChar[5] = {0};
    uint8_t CharLength = 0;
    uint16_t XOffset = 0;
    uint16_t pIndex;

    // 仅支持16x16字体（与OLED字模格式保持一致）
    if (FontSize != LCD_16X16)
        return;

    // 居中显示模式
    if (Center)
    {
        LCD_ShowChinese_Center_Internal(Y, Chinese, FontSize, FColor, BColor);
        return;
    }

    while (Chinese[i] != '\0') // 遍历字符串
    {
#ifdef LCD_CHARSET_UTF8 // 定义字符集为UTF8
        /*此段代码的目的是，提取UTF8字符串中的一个字符，转存到SingleChar子字符串中*/
        /*判断UTF8编码第一个字节的标志位*/
        if ((Chinese[i] & 0x80) == 0x00) // 第一个字节为0xxxxxxx
        {
            CharLength = 1;               // 字符为1字节（ASCII）
            SingleChar[0] = Chinese[i++]; // 将第一个字节写入SingleChar第0个位置，随后i指向下一个字节
            SingleChar[1] = '\0';         // 为SingleChar添加字符串结束标志位
        }
        else if ((Chinese[i] & 0xE0) == 0xC0) // 第一个字节为110xxxxx
        {
            CharLength = 2;               // 字符为2字节
            SingleChar[0] = Chinese[i++]; // 将第一个字节写入SingleChar第0个位置
            if (Chinese[i] == '\0')
            {
                break;
            } // 意外情况，跳出循环
            SingleChar[1] = Chinese[i++]; // 将第二个字节写入SingleChar第1个位置
            SingleChar[2] = '\0';         // 为SingleChar添加字符串结束标志位
        }
        else if ((Chinese[i] & 0xF0) == 0xE0) // 第一个字节为1110xxxx
        {
            CharLength = 3; // 字符为3字节
            SingleChar[0] = Chinese[i++];
            if (Chinese[i] == '\0')
            {
                break;
            }
            SingleChar[1] = Chinese[i++];
            if (Chinese[i] == '\0')
            {
                break;
            }
            SingleChar[2] = Chinese[i++];
            SingleChar[3] = '\0';
        }
        else if ((Chinese[i] & 0xF8) == 0xF0) // 第一个字节为11110xxx
        {
            CharLength = 4; // 字符为4字节
            SingleChar[0] = Chinese[i++];
            if (Chinese[i] == '\0')
            {
                break;
            }
            SingleChar[1] = Chinese[i++];
            if (Chinese[i] == '\0')
            {
                break;
            }
            SingleChar[2] = Chinese[i++];
            if (Chinese[i] == '\0')
            {
                break;
            }
            SingleChar[3] = Chinese[i++];
            SingleChar[4] = '\0';
        }
        else
        {
            i++; // 意外情况，i指向下一个字节，忽略此字节，继续判断下一个字节
            continue;
        }
#endif

#ifdef LCD_CHARSET_GB2312 // 定义字符集为GB2312
        /*此段代码的目的是，提取GB2312字符串中的一个字符，转存到SingleChar子字符串中*/
        /*判断GB2312字节的最高位标志位*/
        if ((Chinese[i] & 0x80) == 0x00) // 最高位为0
        {
            CharLength = 1;               // 字符为1字节（ASCII）
            SingleChar[0] = Chinese[i++]; // 将第一个字节写入SingleChar第0个位置
            SingleChar[1] = '\0';         // 为SingleChar添加字符串结束标志位
        }
        else // 最高位为1
        {
            CharLength = 2;               // 字符为2字节
            SingleChar[0] = Chinese[i++]; // 将第一个字节写入SingleChar第0个位置
            if (Chinese[i] == '\0')
            {
                break;
            } // 意外情况，跳出循环
            SingleChar[1] = Chinese[i++]; // 将第二个字节写入SingleChar第1个位置
            SingleChar[2] = '\0';         // 为SingleChar添加字符串结束标志位
        }
#endif

        /*显示上述代码提取到的SingleChar*/
        if (CharLength == 1) // 如果是单字节字符（ASCII）
        {
            /*使用LCD_ShowChar显示此字符*/
            LCD_ShowChar(X + XOffset, Y, SingleChar[0], LCD_8X16, FColor, BColor);
            XOffset += 8;
        }
        else // 否则，即多字节字符（汉字）
        {
            /*遍历整个字模库，从字模库中寻找此字符的数据*/
            /*如果找到最后一个字符（定义为空字符串），则表示字符未在字模库定义，停止寻找*/
            for (pIndex = 0; strcmp(LCD_CF16x16[pIndex].Index, "") != 0; pIndex++)
            {
                /*找到匹配的字符*/
                if (strcmp(LCD_CF16x16[pIndex].Index, SingleChar) == 0)
                {
                    break; // 跳出循环，此时pIndex的值为指定字符的索引
                }
            }

            // LCD 字模库（与 OLED 共享）是列优先(每列2字节，共16列 -> 32字节)
            // 而 LCD_ShowImage 期望的是行优先（每行按字节存放，(16+7)/8=2 字节/行，共16行）
            // 所以在显示前把 16x16 的列优先字模转换为行优先临时缓冲区
            uint8_t converted[32] = {0}; // 16行 * 2字节/行

            // 遍历每个像素并按行/列重排到 converted
            for (uint8_t row = 0; row < 16; row++)
            {
                for (uint8_t col = 0; col < 16; col++)
                {
                    // 列优先存储约定：前16字节为列0..15 的低8位（rows 0..7），后16字节为列0..15 的高8位（rows 8..15）
                    uint8_t bit;
                    if (row < 8)
                        bit = (LCD_CF16x16[pIndex].Data[col] >> row) & 0x01;
                    else
                        bit = (LCD_CF16x16[pIndex].Data[col + 16] >> (row - 8)) & 0x01;

                    if (bit)
                    {
                        uint16_t idx = row * 2 + (col / 8);
                        converted[idx] |= (1 << (col % 8));
                    }
                }
            }

            // 使用转换后的行优先数据绘制到 LCD
            LCD_ShowImage(X + XOffset, Y, 16, 16, converted, FColor, BColor);
            XOffset += 16;
        }
    }
}
