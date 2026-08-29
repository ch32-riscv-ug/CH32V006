/********************************** (C) COPYRIGHT  *******************************
 * File Name          : control.h
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
#ifndef _CONTROL_H
#define _CONTROL_H

#include "def.h"

extern volatile bool gMotorIsNeedRun;
extern volatile int32_t gMultiSamplingPoint[4];
extern volatile int32_t gMultiSamplingCnt;
extern volatile uint8_t gMultiSamplingPointNow;

extern void MotorControl(void);
extern void EnterRunInit(void);
extern bool StartupDrag(void);
extern void MotorInit(void);
extern void MotorAlign(void);

void MotorZeroCross(void);

#endif
