/********************************** (C) COPYRIGHT  *******************************
 * File Name          : drvuart.h
 * Author             : WCH
 * Version            : V1.0.1
 * Date               : 2026/11/05
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

// DrvUartInit: initialization
void DrvUartInit(void);

// DrvUartSendBytes: send bytes
void DrvUartSendBytes(uint8_t *bytes, uint16_t size);

#endif
