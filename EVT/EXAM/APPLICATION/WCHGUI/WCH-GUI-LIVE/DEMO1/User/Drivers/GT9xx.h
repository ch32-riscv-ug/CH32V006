/********************************** (C) COPYRIGHT  *******************************
* File Name          : GT9xx.h
* Author             : WCH
* Version            : V1.0.0
* Date               : 2025/08/01
* Description        : This file provides some prototypes and defines .
*********************************************************************************
* Copyright (c) 2025 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/


#ifndef __TOUCH_H
#define __TOUCH_H

#include "soft_iic.h"

#define TOUCH_MAX 2

typedef struct
{
    uint8_t  flag;
    uint8_t  num;
    uint16_t x[TOUCH_MAX];
    uint16_t y[TOUCH_MAX];
} TouchStructure;

extern volatile TouchStructure touchInfo;

typedef struct
{
    uint8_t  ready;
    char     id[5];
    uint16_t fw_version;
    uint16_t width;
    uint16_t height;
} TouchChipInfo;

extern TouchChipInfo touchChipInfo;

#define GT9XX_IIC_RADDR 0xBB
#define GT9XX_IIC_WADDR 0xBA

// #define GT9XX_IIC_RADDR 0x29
// #define GT9XX_IIC_WADDR 0x28

#define GT9XX_CFG_ADDR  0x8047
#define GT9XX_READ_ADDR 0x814E
#define GT9XX_ID_ADDR   0x8140

uint8_t Touch_Init(void);
void    Touch_Scan(void);
void    GT9XX_Reset(void);

#endif
