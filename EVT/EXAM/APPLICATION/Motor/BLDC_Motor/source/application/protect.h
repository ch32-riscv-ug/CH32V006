/********************************** (C) COPYRIGHT  *******************************
 * File Name          : protect.h
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
#ifndef PROTECT_H
#define PROTECT_H

#include "common.h"

// 保护模块初始化
void ProtectInit(void);

// 保护模块运行
void ProtectRun(void);

// 实时保护检查
void ProtectRealTimeCheck(void);

#endif
