/********************************** (C) COPYRIGHT  *******************************
 * File Name          : startup.h
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
#ifndef STARTUP_H
#define STARTUP_H

#include "stddef.h"
#include "stdint.h"
#include "stdbool.h"

// StartupLoad 载入
// zeroRatio 过零比例 范围0-100,推荐5
// zeroCnt 过零次数 默认推荐3次
void StartupLoad(uint8_t zeroRatio, uint8_t zeroCnt, bool supportHighSpeed);

// startupCheckZeroCross 检测过零点
bool StartupCheckZeroCross(uint8_t dir, uint8_t phase, uint16_t bemfNow, uint16_t vbus);

// StartupReset 重置启动参数
void StartupReset(void);

// StartupIsUp 判断是否上升沿
bool StartupIsUp(void);

// StartupIsFinish 判断启动是否完成
bool StartupIsFinish(void);

// CheckZeroCross 检测过零点
bool CheckZeroCross(uint8_t dir, uint8_t phase, uint16_t bemfNow, uint16_t vbus);

// StartupIsFirstZeroCrossTriggered 判断首次过零触发信号是否已触发
// 注意：此函数会重置标志位，仅在需要时调用一次
// 返回值：如果首次过零已触发则返回true，否则返回false
bool StartupIsFirstZeroCrossTriggered(void);

#endif
