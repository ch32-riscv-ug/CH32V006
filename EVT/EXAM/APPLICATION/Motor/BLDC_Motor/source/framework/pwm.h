/********************************** (C) COPYRIGHT  *******************************
 * File Name          : pwm.h
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
#ifndef _PWM_H
#define _PWM_H
//#include "timer3.h"
#include "common.h"

#define UP16LIMIT(var, max, min)               \
    {                                          \
        (var) = (var) > (max) ? (max) : (var); \
        (var) = (var) < (min) ? (min) : (var); \
    }
extern FAST_FUNC void PWMSwitchPhase(void);
extern FAST_FUNC void PWMChangeDuty(uint16_t u16data);
extern void PWMSetLonHoff(void);
extern void PWMSetAllOff(void);
extern void PWMCharge(uint8_t phase);
#endif
