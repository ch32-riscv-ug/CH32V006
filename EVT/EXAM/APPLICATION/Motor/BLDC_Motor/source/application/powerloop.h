/********************************** (C) COPYRIGHT  *******************************
 * File Name          : powerloop.h
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

#ifndef POWERLOOP_H
#define POWERLOOP_H

#include "framework.h"

// PowerLoopInit: power loop initialization
void PowerLoopInit(void);

// PowerLoopRun: power loop run
void PowerLoopRun(void);

// PowerLoopReset: power loop reset
void PowerLoopReset(void);

// PowerLoopSetPower: set the target power
// power: target power, unit W
void PowerLoopSetPower(float power);

// PowerLoopGetPower: get the power
// Return: power, unit W
float PowerLoopGetPower(void);

// PowerLoopGetTarget: get the target power
// Return: target power, unit W
float PowerLoopGetTarget(void);

// PowerLoopGetOutput: get the power loop output
// Return: duty cycle in Q15 format
int32_t PowerLoopGetOutput(void);

// PowerLoopSetUI: set the power loop integral value
// ui: integral value
void PowerLoopSetUI(int32_t ui);

// PowerLoopIsSaturated: check whether the power loop is saturated
bool PowerLoopIsSaturated(void);

// PowerLoopSetOutMax: set the power loop maximum output
// outMax: maximum output
void PowerLoopSetOutMax(int32_t outMax);

// PowerLoopSetOutMin: set the power loop minimum output
// outMin: minimum output
void PowerLoopSetOutMin(int32_t outMin);

#endif
