/********************************** (C) COPYRIGHT  *******************************
 * File Name          : launch.h
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
#ifndef LAUNCH_H
#define LAUNCH_H

#include "drivers.h"
#include "adcsample.h"

// Launcher load initialization
void LaunchLoad(void);
// Launcher run loop
void LaunchRun(void);

#endif

