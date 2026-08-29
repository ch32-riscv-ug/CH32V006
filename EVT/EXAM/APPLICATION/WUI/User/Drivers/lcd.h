/*********************************************************************
 * File Name          : lcd.h
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2023/8/15
 * Description        : file for lcd screen.
 *********************************************************************************
* Copyright (c) 2023 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/

#ifndef __LCD_H
#define __LCD_H
#include "spi.h"
#include "stdio.h"
#include "stdlib.h"
#include "flash.h"
#include "config.h"

#define USE_HORIZONTAL 2

#if USE_HORIZONTAL == 0 || USE_HORIZONTAL == 1
#define LCD_W 240
#define LCD_H 320
#endif

#if USE_HORIZONTAL == 2 || USE_HORIZONTAL == 3
#define LCD_W 320
#define LCD_H 240
#endif

#define LCD_WIDTH                 LCD_W
#define LCD_HEIGHT                LCD_H

#define LCD_SWAP16(x)             ((uint16_t)(((x) & 0xFF) << 8) | (((x) >> 8) & 0xFF))

#define RGB888_TO_RGB565(r, g, b) LCD_SWAP16((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3))

#define RGB24_TO_RGB565(rgb)      LCD_SWAP16((((rgb) >> 8) & 0xF800) | (((rgb) >> 5) & 0x07E0) | (((rgb) >> 3) & 0x001F))

#define LCD_red                   RGB888_TO_RGB565(255, 0, 0)
#define LCD_green                 RGB888_TO_RGB565(0, 255, 0)
#define LCD_blue                  RGB888_TO_RGB565(0, 0, 255)
#define LCD_white                 RGB888_TO_RGB565(255, 255, 255)
#define LCD_gray                  RGB888_TO_RGB565(128, 128, 128)
#define LCD_black                 RGB888_TO_RGB565(0, 0, 0)
#define LCD_yellow                RGB888_TO_RGB565(255, 255, 0)

// [15:11] 5bit red
//  [10:5] 6bit green
//   [4:0] 5bit blue
#define LCD_WHITE                 LCD_white
#define LCD_BLACK                 LCD_black
#define LCD_BLUE                  LCD_blue
#define LCD_RED                   LCD_red
#define LCD_MAGENTA               RGB888_TO_RGB565(255, 0, 255)
#define LCD_GREEN                 LCD_green
#define LCD_CYAN                  RGB888_TO_RGB565(0, 255, 255)
#define LCD_YELLOW                LCD_yellow
#define LCD_BROWN                 RGB888_TO_RGB565(165, 42, 42)
#define LCD_GRAY                  LCD_gray

#define LCD_LED_SET()             (LCD_LED_PORT->BSHR = LCD_LED_PIN)
#define LCD_CS_SET()              (LCD_CS_PORT->BSHR = LCD_CS_PIN)
#define LCD_DC_SET()              (LCD_DC_PORT->BSHR = LCD_DC_PIN)
#define LCD_RST_SET()             (LCD_RST_PORT->BSHR = LCD_RST_PIN)

#define LCD_LED_CLR()             (LCD_LED_PORT->BCR = LCD_LED_PIN)
#define LCD_CS_CLR()              (LCD_CS_PORT->BCR = LCD_CS_PIN)
#define LCD_DC_CLR()              (LCD_DC_PORT->BCR = LCD_DC_PIN)
#define LCD_RST_CLR()             (LCD_RST_PORT->BCR = LCD_RST_PIN)

#define LCD_RS_CLR()              LCD_DC_CLR()
#define LCD_RS_SET()              LCD_DC_SET()

void LCD_Init(void);
void LCD_Clear(uint16_t Color);
void LCD_SetWindows(uint16_t xStar, uint16_t yStar, uint16_t xEnd, uint16_t yEnd);

#endif
