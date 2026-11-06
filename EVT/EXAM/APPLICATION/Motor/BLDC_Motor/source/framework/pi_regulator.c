/********************************** (C) COPYRIGHT  *******************************
 * File Name          : pi_regulator.c
 * Author             : WCH
 * Version            : V1.0.1
 * Date               : 2026/11/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "pi_regulator.h"

#define _IQnmpy(a, b, c) ((a * b) >> c)

static void series(PIParam *param, int32_t ref, int32_t fbk);
static void parallel(PIParam *param, int32_t ref, int32_t fbk);

// PISetParam: set the PI controller parameters
void PISetParam(PIParam *param, float kp, float ki, float outMax, float outMin) {
    param->UPShift = 15;
    param->UIShift = 15;
    param->KP = _IQ15(kp);
    param->KI = _IQ15(ki);
    param->OutMax = _IQ15(outMax);
    param->OutMin = _IQ15(outMin);
    // When the gain exceeds 1.0, halve K and decrease the shift count accordingly, keeping K*Err from overflowing int32.
    // The Shift > 0 lower bound guards against undefined behavior from a negative shift count
    while (param->KP > _IQ15(1.0f) && param->UPShift > 0) {
        param->KP >>= 1;
        param->UPShift--;
    }
    while (param->KI > _IQ15(1.0f) && param->UIShift > 0) {
        param->KI >>= 1;
        param->UIShift--;
    }

    PIReset(param);
}

/**
 * @brief PI controller reset
 * @param param PI structure
 * @return None
 * @date 2021-06-03
 * @version 1.0 
 */
void PIReset(PIParam *param) {
    param->Err = 0;
    param->UP = 0;
    param->UI = 0;
    param->UIFrac = 0;
    param->Out = 0;
}

// PISetUI: set the integral term directly (for bumpless transfer)
void PISetUI(PIParam *param, int32_t ui) {
    param->UI = ui;
    param->UIFrac = 0;
}

// integrate accumulates the Q(UIShift)-scaled integral increment into UI
// delta: KI * Err (parallel type) or KI * UP (series type), not yet shifted right
//
// The original implementation was UI += delta >> UIShift, whose quotient is always 0 when KI is small. Taking this project's speed loop as an example:
//   KI = _IQ15(0.0025) = 81, UIShift = 15  =>  only |Err| >= 32768/81 = 405 (Q15) yields 1 LSB
//   405 Q15 with SPEED_MAX=100000 eRPM is an integral dead zone of 1236 eRPM,
//   which at a 10000 eRPM setpoint means a 12.4% steady-state error that is never integrated away.
// Now the truncated remainder is kept in UIFrac and accumulated across periods, the dead zone becomes |Err| >= 1,
// and the integration rate is again determined by KI rather than by the Q15 resolution of UI.
static inline void integrate(PIParam *param, int32_t delta) {
    // acc upper bound: |UIFrac| < 2^15, |delta| = |KI * Err| <= 2^15 * 2^15, so int32 does not overflow
    int32_t acc = param->UIFrac + delta;
    int32_t step = acc >> param->UIShift; // Arithmetic right shift, negative values round down
    param->UI += step;
    // Take a non-negative remainder. Equivalent to acc - (step << UIShift), but avoids undefined behavior from left-shifting negatives
    param->UIFrac = acc & ((1 << param->UIShift) - 1);
}

static void series(PIParam *param, int32_t ref, int32_t fbk) {
    param->Err = ref - fbk;
    param->UP = _IQnmpy(param->KP, param->Err, param->UPShift);
    integrate(param, param->KI * param->UP);
    param->UI = _IQsat(param->UI, param->OutMax, param->OutMin);
    param->Out = _IQsat(param->UP + param->UI, param->OutMax, param->OutMin);
}

static void parallel(PIParam *param, int32_t ref, int32_t fbk) {
    param->Err = ref - fbk;
    param->UP = _IQnmpy(param->KP, param->Err, param->UPShift);
    integrate(param, param->KI * param->Err);
    param->UI = _IQsat(param->UI, param->OutMax, param->OutMin);
    param->Out = _IQsat(param->UP + param->UI, param->OutMax, param->OutMin);
}

// PICalc: PI controller calculation
// When only an error is supplied, ref = err, fbk = 0
int32_t PICalc(PIParam *param, int32_t ref, int32_t fbk) {
    if (param->Mode == PI_MODE_PARALLEL) {
        parallel(param, ref, fbk);
    } else if (param->Mode == PI_MODE_SERIES) {
        series(param, ref, fbk);
    }
    return param->Out;
}
