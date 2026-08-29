/********************************** (C) COPYRIGHT  *******************************
 * File Name          : drvkey.h
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
#ifndef DRVKEY_H
#define DRVKEY_H

#include "def.h"

#define DRVKEY_PIN GPIO_Pin_3
#define DRVKEY_PORT GPIOB

// 按键初始化
void DrvKeyInit(void);

// 按键获取
bool DrvKeyGet(void);

#endif
