/********************************** (C) COPYRIGHT  *******************************
 * File Name          : pi_regulator.h
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
#ifndef PI_REGULATOR_H
#define PI_REGULATOR_H

#include "def.h"

typedef enum {
    PI_MODE_PARALLEL = 0,
    PI_MODE_SERIES,
} PIMode;

typedef struct {
    PIMode Mode;
    int32_t KP;
    int32_t KI;
    int32_t UP;
    int32_t UI;
    // Integrator sub-LSB remainder, in units of Q(UIShift).
    // UI itself only has Q15 resolution, so with a small KI the single increment (KI*Err)>>UIShift truncates to 0 and the
    // integrator does not move at all while |Err| < (1<<UIShift)/KI - the steady-state error can never be eliminated.
    // The truncated fraction is kept here and accumulated across periods until a whole LSB carries into UI.
    int32_t UIFrac;
    int32_t UPShift;
    int32_t UIShift;
    int32_t OutMax;
    int32_t OutMin;
    int32_t Out;
    int32_t Err;
} PIParam;

// PISetParam: set the PI controller parameters
void PISetParam(PIParam *param, float kp, float ki, float outMax, float outMin);

// PIReset: reset the PI controller
void PIReset(PIParam *param);

// PISetUI: set the integral term directly (for bumpless transfer)
// Also clears the sub-LSB remainder so leftover fractions from the previous run do not carry immediately after the switch
void PISetUI(PIParam *param, int32_t ui);

// PICalc: PI controller calculation
// When only an error is supplied, ref = err, fbk = 0
int32_t PICalc(PIParam *param, int32_t ref, int32_t fbk);

#endif
