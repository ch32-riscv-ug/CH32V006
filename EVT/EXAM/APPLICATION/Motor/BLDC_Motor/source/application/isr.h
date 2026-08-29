/********************************** (C) COPYRIGHT  *******************************
 * File Name          : isr.h
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
#ifndef ISR_H
#define ISR_H

#include "def.h"

// SysTimeGet 获取系统时间
uint64_t SysTimeGet(void);

// MotorStart 启动电机
void MotorStart(void);

// MotorStop 停止电机
void MotorStop(void);

// MotorBrake 线性刹车
void MotorBrake(void);

// MotorSetDuty 设置占空比
void MotorSetDuty(uint16_t duty);

#endif
