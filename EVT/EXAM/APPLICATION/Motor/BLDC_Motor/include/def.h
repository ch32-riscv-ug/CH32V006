/********************************** (C) COPYRIGHT  *******************************
 * File Name          : def.h
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
#ifndef DEF_H
#define DEF_H

#include "ch32v00X.h"

#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include "string.h"

#include "config.h"

#include "tztype.h"
#include "tztime.h"
#include "key.h"
#include "pt.h"

#include "IQmath_RV32.h"

#include "pi_regulator.h"

#define ABS(x) ((x) < 0 ? -(x) : (x))

#endif
