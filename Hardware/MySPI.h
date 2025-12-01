#ifndef __MYSPI_H__
#define __MYSPI_H__

#include "stm32f10x.h"

#define LCD_SPI SPI1
#define LCD_SPI_CLK RCC_APB2Periph_SPI1
#define LCD_CS_PIN GPIO_Pin_3
#define LCD_CS_PORT GPIOA
#define LCD_RS_PIN GPIO_Pin_6
#define LCD_RS_PORT GPIOA
#define LCD_RST_PIN GPIO_Pin_4
#define LCD_RST_PORT GPIOA
#define LCD_SCK_PIN GPIO_Pin_5  // SPI1_SCK
#define LCD_MOSI_PIN GPIO_Pin_7 // SPI1_MOSI

#define W25Q64_SPI SPI2
#define W25Q64_SPI_CLK RCC_APB1Periph_SPI2
#define W25Q64_GPIO_PORT RCC_APB2Periph_GPIOB
#define W25Q64_CS_PIN GPIO_Pin_12
#define W25Q64_CS_PORT GPIOB
#define W25Q64_CLK_PIN GPIO_Pin_13
#define W25Q64_CLK_PORT GPIOB
#define W25Q64_DO_PIN GPIO_Pin_14
#define W25Q64_DO_PORT GPIOB
#define W25Q64_DI_PIN GPIO_Pin_15
#define W25Q64_DI_PORT GPIOB

void MySPI1_Init(void);
void MySPI2_Init(void);
void MySPI2_Start(void);
void MySPI2_Stop(void);
uint8_t MySPI2_SwapByte(uint8_t ByteSend);

#endif
