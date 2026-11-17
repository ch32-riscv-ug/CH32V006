#include "lcd.h"

#define LCD_FILL_BUF_PIXELS 1024

/* 半字缓冲：直接存标准 RGB565，16 位 SPI 帧会按高字节先发 */
static uint16_t lcd_fill_buf[LCD_FILL_BUF_PIXELS];

/*
 * 16 位帧轮询发送
 * 一次写一个像素，仅作为 DMA 失败时的回退
 */
static void lcd_fill_stream_poll(const uint16_t *buf, uint32_t count)
{
    uint32_t i;

    for (i = 0; i < count; i++)
    {
        while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET)
        {
        }
        SPI2->DATAR = buf[i];
    }

    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_BSY) == SET)
    {
    }
}

/*
 * 半字 DMA 发送，带超时保护
 * count 单位是像素（半字），不是字节
 */
static uint8_t lcd_fill_dma_send(const uint16_t *buf, uint32_t count)
{
    uint32_t timeout = 0x100000;

    // while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_BSY) == SET)
    // {
    //     if (--timeout == 0)
    //     {
    //         return 0;
    //     }
    // }

    // timeout = 0x100000;
    SPI2_Write_DMA16((uint16_t *)buf, (uint16_t)count);

    while (SPI2_DMA_TX_CH->CNTR > 0)
    {
        if (--timeout == 0)
        {
            DMA_Cmd(SPI2_DMA_TX_CH, DISABLE);
            return 0;
        }
    }



    return 1;
}

void LCD_Fill(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t Color)
{
    uint32_t i;
    uint32_t total;
    uint32_t offset;
    uint32_t chunk;
    uint8_t  use_dma = 1;

    if (x2 >= LCD_W)
    {
        x2 = LCD_W - 1;
    }

    if (y2 >= LCD_H)
    {
        y2 = LCD_H - 1;
    }

    if (x1 > x2 || y1 > y2)
    {
        return;
    }

    /* 像素总数，单位为半字 */
    total = (uint32_t)(x2 - x1 + 1) * (uint32_t)(y2 - y1 + 1);

    for (i = 0; i < LCD_FILL_BUF_PIXELS; i++)
    {
        lcd_fill_buf[i] = Color;
    }

    /* 设置窗口和写 GRAM 命令仍使用 8 位帧 */
    LCD_SetWindows(x1, y1, x2, y2);

    /* 像素阶段切换到 16 位帧，半字 DMA 会自动按高字节先发 */
    SPI2_SetDataSize(SPI_DataSize_16b);

    LCD_CS_CLR();
    LCD_DC_SET();

    offset = 0;

    while (offset < total)
    {
        chunk = total - offset;

        if (chunk > LCD_FILL_BUF_PIXELS)
        {
            chunk = LCD_FILL_BUF_PIXELS;
        }

        if (use_dma && lcd_fill_dma_send(lcd_fill_buf, chunk))
        {
        }
        else
        {
            use_dma = 0;
            lcd_fill_stream_poll(lcd_fill_buf, chunk);
        }

        offset += chunk;
    }

    /* 等最后一个半字真正移出，再拉高 CS */
    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET)
    {
    }

    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_BSY) == SET)
    {
    }

    LCD_CS_SET();

    /* 恢复 8 位帧，后续命令才能正常发送 */
    SPI2_SetDataSize(SPI_DataSize_8b);
}
