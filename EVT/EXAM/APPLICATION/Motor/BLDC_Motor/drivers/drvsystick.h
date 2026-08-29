/********************************** (C) COPYRIGHT  *******************************
 * File Name          : drvsystick.h
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
#ifndef DRVSYSTICK_H
#define DRVSYSTICK_H

#include "def.h"

// DrvSystickInit 驱动初始化
void DrvSystickInit(void);

// DrvSystickCmd 使能或失能滴答时钟
// enable 使能
// 无论使能还是使能都会设置CNT为0
void DrvSystickCmd(bool enable);


// DrvSystickClear 清除滴答时钟
void DrvSystickClear(void);

#endif
