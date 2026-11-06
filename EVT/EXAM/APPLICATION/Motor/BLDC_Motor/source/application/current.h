/********************************** (C) COPYRIGHT  *******************************
 * File Name          : current.h
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

#ifndef CURRENT_H
#define CURRENT_H

#include "framework.h"

// CurrentInit: current limit loop initialization
void CurrentInit(void);

// CurrentRun: current limit loop run
void CurrentRun(void);

// CurrentReset: current limit loop reset
void CurrentReset(void);

// CurrentSetLimit: set the current limit
// current: current limit, unit A
void CurrentSetLimit(float current);

// CurrentGetLimit: get the current limit
// Return: current limit, unit A
float CurrentGetLimit(void);

// CurrentGetOutput: get the current limit loop output
// Return: duty cycle in Q15 format
int32_t CurrentGetOutput(void);

// CurrentIsLimiting: check whether limiting is active
// Return: true - limiting, false - not limiting
bool CurrentIsLimiting(void);

// CurrentIsSaturated: check whether the current limit loop output is saturated
// Return: true - duty cycle has reached the upper limit and the current is below the limit
bool CurrentIsSaturated(void);

// CurrentSetUI: set the current loop integral value
// ui: integral value
void CurrentSetUI(int32_t ui);

// CurrentSetOutMax: set the current loop maximum output
// outMax: maximum output
void CurrentSetOutMax(int32_t outMax);

// CurrentSetOutMin: set the current loop minimum output
// outMin: minimum output
void CurrentSetOutMin(int32_t outMin);

#endif
