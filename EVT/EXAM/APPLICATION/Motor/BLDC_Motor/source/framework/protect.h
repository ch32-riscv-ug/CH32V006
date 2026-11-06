/********************************** (C) COPYRIGHT  *******************************
 * File Name          : protect.h
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

#ifndef PROTECT_H
#define PROTECT_H

#include "interface.h"

// Protection module initialization
void ProtectInit(void);

// Protection module run
void ProtectRun(void);

// Real-time protection check
void ProtectRealTimeCheck(void);

void ProtectSetError(uint16_t errorMask);
void ProtectClearErrorMask(uint16_t errorMask);
void ProtectClearError(void);

#endif
