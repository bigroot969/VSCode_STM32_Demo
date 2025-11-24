#include "MySPI.h"

void MySPI1_Init(void)
{
    RCC_APB2PeriphClockCmd(LCD_SPI_CLK, ENABLE);
    SPI_InitTypeDef SPI_InitStruct;
    SPI_InitStruct.SPI_Direction = SPI_Direction_1Line_Tx;          // 单向发送
    SPI_InitStruct.SPI_Mode = SPI_Mode_Master;                      // 主机模式
    SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;                  // 8 位数据
    SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;                         // 时钟极性：低电平空闲
    SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;                       // 时钟相位：第一个边沿采样
    SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;                          // 软件控制 NSS（CS 自行管理）
    SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8; // 72MHz/8=9MHz（符合 ST7735S 上限）
    SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;                 // 高位先行
    SPI_InitStruct.SPI_CRCPolynomial = 7;

    SPI_Init(LCD_SPI, &SPI_InitStruct);
    SPI_Cmd(LCD_SPI, ENABLE);
}

/**
  * 函    数：SPI写SS引脚电平，SS仍由软件模拟
  * 参    数：BitValue 协议层传入的当前需要写入SS的电平，范围0~1
  * 返 回 值：无
  * 注意事项：此函数需要用户实现内容，当BitValue为0时，需要置SS为低电平，当BitValue为1时，需要置SS为高电平
  */
void MySPI2_W_SS(uint8_t BitValue)
{
	GPIO_WriteBit(W25Q64_CS_PORT, W25Q64_CS_PIN, (BitAction)BitValue);		//根据BitValue，设置SS引脚的电平
}

/**
  * 函    数：SPI初始化
  * 参    数：无
  * 返 回 值：无
  */
void MySPI2_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(W25Q64_GPIO_PORT, ENABLE);	//开启GPIOB的时钟
	RCC_APB1PeriphClockCmd(W25Q64_SPI_CLK, ENABLE);	//开启SPI2的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	// CS引脚 - PB12推挽输出(软件控制片选)
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = W25Q64_CS_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(W25Q64_CS_PORT, &GPIO_InitStructure);
	
	// SCK(PB13)和MOSI(PB15) - 复用推挽输出
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = W25Q64_CLK_PIN | W25Q64_DI_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(W25Q64_CLK_PORT, &GPIO_InitStructure);
	
	// MISO(PB14) - 上拉输入
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = W25Q64_DO_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(W25Q64_DO_PORT, &GPIO_InitStructure);
	
	/*SPI初始化*/
	SPI_InitTypeDef SPI_InitStructure;						//定义结构体变量
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;			//模式，选择为SPI主模式
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;	//方向，选择2线全双工
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;		//数据宽度，选择为8位
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;		//先行位，选择高位先行
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128;	//波特率分频，选择128分频
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;				//SPI极性，选择低极性
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;			//SPI相位，选择第一个时钟边沿采样，极性和相位决定选择SPI模式0
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;				//NSS，选择由软件控制
	SPI_InitStructure.SPI_CRCPolynomial = 7;				//CRC多项式，暂时用不到，给默认值7
	SPI_Init(W25Q64_SPI, &SPI_InitStructure);						//将结构体变量交给SPI_Init，配置SPI2
	
	/*SPI使能*/
	SPI_Cmd(W25Q64_SPI, ENABLE);									//使能SPI2，开始运行
	
	/*设置默认电平*/
	MySPI2_W_SS(1);											//SS默认高电平
}

/**
  * 函    数：SPI起始
  * 参    数：无
  * 返 回 值：无
  */
void MySPI2_Start(void)
{
	MySPI2_W_SS(0);				//拉低SS，开始时序
}

/**
  * 函    数：SPI终止
  * 参    数：无
  * 返 回 值：无
  */
void MySPI2_Stop(void)
{
	MySPI2_W_SS(1);				//拉高SS，终止时序
}

/**
  * 函    数：SPI交换传输一个字节，使用SPI模式0
  * 参    数：ByteSend 要发送的一个字节
  * 返 回 值：接收的一个字节
  */
uint8_t MySPI2_SwapByte(uint8_t ByteSend)
{
	while (SPI_I2S_GetFlagStatus(W25Q64_SPI, SPI_I2S_FLAG_TXE) != SET);	//等待发送数据寄存器空
	
	SPI_I2S_SendData(W25Q64_SPI , ByteSend);								//写入数据到发送数据寄存器，开始产生时序
	
	while (SPI_I2S_GetFlagStatus(W25Q64_SPI, SPI_I2S_FLAG_RXNE) != SET);	//等待接收数据寄存器非空
	
	return SPI_I2S_ReceiveData(W25Q64_SPI);								//读取接收到的数据并返回
}
