/********************************** (C) COPYRIGHT  *******************************
 * File Name          : framework.h
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

#ifndef FRAMEWORK_H
#define FRAMEWORK_H

// IWYU pragma: begin_exports
#include "drivers.h"
#include "critical.h"
#include "halless.h"
#include "interface.h"
#include "key.h"
#include "pi_regulator.h"
#include "protect.h"
#include "pwm.h"
#include "IQmath_RV32.h"
#include "param.h"
// IWYU pragma: end_exports

// FrameworkLoad: framework load
void FrameworkLoad(void);

// FrameworkRun: framework run
void FrameworkRun(void);

#endif
