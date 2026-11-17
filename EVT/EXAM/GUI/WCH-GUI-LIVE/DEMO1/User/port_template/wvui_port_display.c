

#include "wvui_port_display.h"

#include "lcd.h"
#include "spi.h"



int lcd_init(void)
{

    LCD_Init();
    return 0;
}

void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    LCD_SetWindows(x0, y0, x1, y1);
    SPI2_SetDataSize(SPI_DataSize_16b);
}

static void lcd_dma_wait(void)
{
    uint32_t timeout;

    timeout = 0x100000u;
    while (SPI2_DMA_TX_CH->CNTR > 0u)
    {
        if (--timeout == 0u)
        {
            DMA_Cmd(SPI2_DMA_TX_CH, DISABLE);
            break;
        }
    }

    timeout = 0x100000u;
    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_BSY) == SET)
    {
        if (--timeout == 0u)
        {
            break;
        }
    }
}

void lcd_write_pixels(const uint16_t *pixels, uint32_t count)
{
    uint16_t n;

    lcd_dma_wait();

    LCD_CS_CLR();
    LCD_DC_SET();

    while (count > 0u)
    {
        n = (count > 0xFFFFu) ? (uint16_t)0xFFFFu : (uint16_t)count;

        SPI2_Write_DMA16((uint16_t *)pixels, n);

        pixels += n;
        count -= n;

        if (count > 0u)
        {
            lcd_dma_wait();
        }
    }
}

void lcd_wait_done(void)
{
    lcd_dma_wait();
}

void lcd_end_transaction(void)
{
    lcd_dma_wait();

    LCD_CS_SET();
    SPI2_SetDataSize(SPI_DataSize_8b);
}


static uint8_t s_ready;

static void tmpl_set_window(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    lcd_set_window((uint16_t)x0, (uint16_t)y0, (uint16_t)x1, (uint16_t)y1);
}

static void tmpl_flush_begin(void)
{
}

static void tmpl_flush(const void *pixels, uint32_t count)
{
    lcd_write_pixels((const uint16_t *)pixels, count);
}

static void tmpl_flush_wait(void)
{
    lcd_wait_done();
}

static void tmpl_flush_end(void)
{
    lcd_end_transaction();
}


static wvui_port_t s_port =
{
    .set_window = tmpl_set_window,
    .flush_begin = tmpl_flush_begin,
    .flush = tmpl_flush,
    .flush_wait = tmpl_flush_wait,
    .flush_end = tmpl_flush_end,
    .async = 0u,
    .align_x = 1u,
    .align_y = 1u,
    .paged = 0u,
    .bus_bytes_per_ms = 0u
};

void wvui_port_display_configure(uint8_t async)
{
    s_port.async = (uint8_t)(async != 0u);
    s_ready = 1u;
}

const wvui_port_t *wvui_port_display(void)
{
    return (s_ready != 0u) ? &s_port : 0;
}
