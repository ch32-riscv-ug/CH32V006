/********************************** (C) COPYRIGHT  *******************************
 * File Name          : pwm.h
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
#ifndef _PWM_H
#define _PWM_H

#include "interface.h"

#define UP16LIMIT(var, max, min)               \
    {                                          \
        (var) = (var) > (max) ? (max) : (var); \
        (var) = (var) < (min) ? (min) : (var); \
    }

// PWM period register value (run-time variable)
extern volatile uint16_t PwmPeriod;
extern volatile uint32_t PwmPeriodUs;

// Convert a Q15 per-unit value to an absolute timer count
#define PWM_Q15_TO_CNT(q15val) ((uint16_t)_IQ15mpy((q15val), (int32_t)PwmPeriod))

extern FAST_FUNC void PWMSwitchPhase(void);
extern FAST_FUNC void PWMApplyPhase(void);
extern FAST_FUNC void PWMSetPhase(uint8_t hphase, uint8_t lphase);
extern FAST_FUNC void PWMChangeDuty(void);
extern FAST_FUNC void PWMTest(void);

extern void PWMSetFrequency(uint32_t freqHz);
extern void PWMSetLonHoff(void);
extern void PWMSetAllOff(void);
extern void PWMCharge(uint8_t phase);
#endif
