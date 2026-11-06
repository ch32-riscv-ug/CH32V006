/********************************** (C) COPYRIGHT  *******************************
 * File Name          : interface.h
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

#ifndef INTERFACE_H
#define INTERFACE_H

#include "drivers.h"

#include "stddef.h"
#include "stdint.h"

// SysTick counter value
#define SYSTICK_CNT SysTick->CNT

// SysTick compare value
#define SYSTICK_CMP SysTick->CMP

// Clear the SysTick counter
#define SYSTICK_CLEAR() SysTick->CNT = 0

// AdcTriggerPoint
#define ADC_TRIGGER_POINT(cnt) TIM3->TIM3_CH1CVR = (uint16_t)(cnt)

#endif
