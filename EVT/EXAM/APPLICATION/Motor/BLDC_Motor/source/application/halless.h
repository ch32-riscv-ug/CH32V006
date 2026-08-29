/********************************** (C) COPYRIGHT  *******************************
 * File Name          : halless.h
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
#ifndef _BEMF_H
#define _BEMF_H
// #include "vc.h"

#include "drivers.h"

// 过零点检测
extern bool CheckZeroCrossing(void);

// HallessReset 复位
void HallessReset(void);

#endif
