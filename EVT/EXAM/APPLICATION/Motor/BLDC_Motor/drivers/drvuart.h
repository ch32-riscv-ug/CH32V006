/********************************** (C) COPYRIGHT  *******************************
 * File Name          : drvuart.h
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2026/08/05
 * Description        : This file contains all the functions prototypes for the
 *                       firmware library.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef DRVUART_H
#define DRVUART_H

#include "def.h"

// DrvUartInit 初始化
void DrvUartInit(void);

// DrvUartSendByte 发送字节
void DrvUartSendBytes(uint8_t *bytes, uint16_t size);

#endif
