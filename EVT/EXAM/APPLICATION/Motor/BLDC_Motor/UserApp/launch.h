/********************************** (C) COPYRIGHT  *******************************
 * File Name          : launch.h
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
#ifndef LAUNCH_H
#define LAUNCH_H

#include "drivers.h"
#include "adcsample.h"

// 启动器载入
void LaunchLoad(void);
// 启动器运行
void LaunchRun(void);

#endif

