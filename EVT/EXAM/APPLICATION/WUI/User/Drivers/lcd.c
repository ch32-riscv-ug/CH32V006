/*********************************************************************
 * File Name          : lcd.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2026/08/27
 * Description        : file for lcd screen.
 *********************************************************************************
* Copyright (c) 2023 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/

#include "lcd.h"
#include "debug.h"
#include "LCD_Font.h"

#define LCD_SOFT_RESET          (0x01)
#define LCD_READ_ID             (0x04)
#define LCD_READ_DISPLAY_STATUS (0x09)
#define LCD_ON                  (0x28)
#define LCD_OFF                 (0x29)

#define LCD_SET_X               (0x2a)
#define LCD_SET_Y               (0x2b)
#define LCD_MEM_WRITE           (0x2C)
#define LCD_MEM_READ            (0x2e)

extern void SPI1_Write(uint8_t data);

static void LCD_WR_REG(uint8_t data)
{
    LCD_DC_CLR();
    SPI1_Write(data);
}

static void LCD_WR_DATA(uint8_t data)
{
    LCD_DC_SET();
    SPI1_Write(data);
}

/*********************************************************************
 * @fn      Lcd_WriteData_16Bit
 * 
 * @brief   write two bytes to the lcd screen
 * 
 * @param   Data - the data to write
 * 
 * @return  none
 */
void Lcd_WriteData_16Bit(uint16_t Data)
{
    SPI1_Write(Data >> 8);
    SPI1_Write(Data);
}

/*********************************************************************
 * @fn      LCD_GPIOInit
 * 
 * @brief   Configuring the control gpio for the lcd screen
 * 
 * @return  none
 */
void LCD_GPIOInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    RCC_PB2PeriphClockCmd(LCD_LED_RCC, ENABLE);
    RCC_PB2PeriphClockCmd(LCD_DC_RCC, ENABLE);
    RCC_PB2PeriphClockCmd(LCD_RST_RCC, ENABLE);
    RCC_PB2PeriphClockCmd(LCD_CS_RCC, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = LCD_LED_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(LCD_LED_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin   = LCD_DC_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(LCD_DC_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin   = LCD_RST_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(LCD_RST_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin   = LCD_CS_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(LCD_CS_PORT, &GPIO_InitStructure);

    LCD_CS_SET();
}

/*********************************************************************
 * @fn      LCD_RESET
 * 
 * @brief   reset the lcd screen by the rst pin
 * 
 * @return  none
 */
void LCD_RESET(void)
{
    LCD_RST_CLR();
    Delay_Ms(100);
    LCD_RST_SET();
    Delay_Ms(50);
}

static const uint8_t lcd_st7789_init_cmds[] = {

    1,
    0x3A,
    0x05,

    2,
    0xB0,
    0x00,
    0xF0,

    5,
    0xB2,
    0x0C,
    0x0C,
    0x00,
    0x33,
    0x33,

    1,
    0xB7,
    0x35,

    1,
    0xBB,
    0x19,

    1,
    0xC0,
    0x2C,

    1,
    0xC2,
    0x01,

    1,
    0xC3,
    0x12,

    1,
    0xC4,
    0x20,

    1,
    0xC6,
    0x0F,

    2,
    0xD0,
    0xA4,
    0xA1,

    14,
    0xE0,
    0xD0,
    0x04,
    0x0D,
    0x11,
    0x13,
    0x2B,
    0x3F,
    0x54,
    0x4C,
    0x18,
    0x0D,
    0x0B,
    0x1F,
    0x23,

    14,
    0xE1,
    0xD0,
    0x04,
    0x0C,
    0x11,
    0x13,
    0x2C,
    0x3F,
    0x44,
    0x51,
    0x2F,
    0x1F,
    0x1F,
    0x20,
    0x23,

    0,
    0x21,

    1,
    0x36,
#if USE_HORIZONTAL == 0
    0,
#elif USE_HORIZONTAL == 1
    (1 << 7) | (1 << 6) | (0 << 5),
#elif USE_HORIZONTAL == 2
    (1 << 6) | (0 << 7) | (1 << 5),
#elif USE_HORIZONTAL == 3
    (1 << 7) | (1 << 5),
#endif

    0,
    0x11,

    0,
    0x29,

    0xff,
    0xff,
};

void LCD_exec_init(uint8_t* init_code)
{
    uint8_t* p = init_code;

    uint8_t data_len;
    uint8_t cmd;

    LCD_CS_CLR();
    while (1)
    {
        data_len = *p++;
        cmd      = *p++;

        if (cmd == 0xFF)
        {
            break;
        }

        LCD_WR_REG(cmd);

        for (uint8_t i = 0; i < data_len; i++)
        {
            LCD_WR_DATA(*p++);
        }
    }
    LCD_CS_SET();
}

void LCD_Init(void)
{

    SPI1_Init();
    LCD_GPIOInit();

    LCD_RESET();

    LCD_LED_SET();

    LCD_RESET();

    LCD_CS_CLR();

    LCD_WR_REG(0x11);

    LCD_exec_init((uint8_t*)lcd_st7789_init_cmds);

    Delay_Ms(100);
}

/*********************************************************************
 * @fn      LCD_SetWindows
 * 
 * @brief   Setting LCD display window
 * 
 * @param   xStar -  the bebinning x coordinate of the LCD display window
 *          yStar -  the bebinning y coordinate of the LCD display window
 *          xEnd -  the endning x coordinate of the LCD display window
 *          yEnd -  the endning y coordinate of the LCD display window
 * 
 * @return  none
 */
void LCD_SetWindows(uint16_t xStar, uint16_t yStar, uint16_t xEnd, uint16_t yEnd)
{
    LCD_CS_CLR();
    LCD_WR_REG(LCD_SET_X);

    LCD_WR_DATA(xStar >> 8);
    LCD_WR_DATA(0x00FF & xStar);
    LCD_WR_DATA(xEnd >> 8);
    LCD_WR_DATA(0x00FF & xEnd);

    LCD_WR_REG(LCD_SET_Y);

    LCD_WR_DATA(yStar >> 8);
    LCD_WR_DATA(0x00FF & yStar);
    LCD_WR_DATA(yEnd >> 8);
    LCD_WR_DATA(0x00FF & yEnd);
    LCD_WR_REG(LCD_MEM_WRITE);

    LCD_CS_SET();
}

void SPI_Flash_Read_wait_dma(void)
{
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET)
        ;
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET)
        ;
}

void flash_read_dma(uint8_t* pBuffer, uint32_t rd_addr, uint16_t count)
{
    void SPI1_Read_DMA(uint8_t* data, uint16_t len);
    void SPI_Flash_Read_dma_start(uint32_t ReadAddr);
    void SPI_Flash_Read_dma_end(void);
    void SPI_Flash_Read_wait_dma(void);

    SPI_Flash_Read_dma_start(rd_addr);
    SPI1_Read_DMA(pBuffer, count);
    SPI_Flash_Read_wait_dma();
    SPI_Flash_Read_dma_end();
}

void lcd_write_dma(uint8_t* pBuffer, uint16_t count)
{

    LCD_CS_CLR();

    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET)
        ;

    DMA_Cmd(SPI1_DMA_TX_CH, DISABLE);
    DMA_ClearFlag(SPI1_DMA_TC_FLAG);
    {
        // lcd dma cfg
        SPI1_DMA_TX_CH->CFGR = 0x2090;
    }
    SPI1_DMA_TX_CH->MADDR = (uint32_t)pBuffer;
    SPI1_DMA_TX_CH->CNTR  = count;

    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
    DMA_Cmd(SPI1_DMA_TX_CH, ENABLE);
}

void LCD_Fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

void LCD_Clear(uint16_t color)
{
    LCD_Fill(0, 0, LCD_W, LCD_H, color);
}