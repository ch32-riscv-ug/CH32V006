/********************************** (C) COPYRIGHT  *******************************
 * File Name          : speed.h
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
#ifndef SPEED_H
#define SPEED_H

#include "common.h"

// SpeedInit 初始化
void SpeedInit(void);

// SpeedRun 运行
void SpeedRun(void);

// SpeedSetTime 设置时间
void SpeedSetTime(uint32_t time);

// SpeedSet 设置转速
void SpeedSet(uint32_t eRPM);

// SpeedSetParam 设置参数
void SpeedSetParam(float kp, float ki, float outMax, float outMin);

// SpeedGet 获取转速
uint32_t SpeedGet(void);

// SpeedReset 重置
void SpeedReset(void);

#endif
