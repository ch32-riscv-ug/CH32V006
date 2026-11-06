/********************************** (C) COPYRIGHT  *******************************
 * File Name          : powerloop.c
 * Author             : WCH
 * Version            : V1.0.1
 * Date               : 2026/11/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#include "powerloop.h"
#include "config.h"
#include "pwm.h"

// Guarded defaults for the power loop parameters
#ifndef POWER_KP
#define POWER_KP 0.5f
#endif
#ifndef POWER_KI
#define POWER_KI 0.1f
#endif
#ifndef POWER_OUT_MAX
#define POWER_OUT_MAX 1.0f
#endif
#ifndef POWER_OUT_MIN
#define POWER_OUT_MIN 0.0f
#endif
#ifndef POWER_TARGET
#define POWER_TARGET 100.0f
#endif
#ifndef POWER_MAX
#define POWER_MAX (VOLTAGE_MAX * CURRENT_MAX)
#endif

static PIParam gPowerPI;
static int32_t gPowerTarget = 0;

// PowerLoopInit: power loop initialization
void PowerLoopInit(void) {
    gPowerPI.Mode = PI_MODE_PARALLEL;
    PISetParam(&gPowerPI, POWER_KP, POWER_KI, POWER_OUT_MAX, POWER_OUT_MIN);
    gPowerTarget = _IQ15(POWER_TARGET / POWER_MAX);
}

// PowerLoopRun: power loop run
void PowerLoopRun(void) {
    // Compute the actual power P = V * I (IQ15 format); use the IIR-filtered CurrentFilter to suppress noise
    int32_t voltage = ADCSample.Voltage;
    int32_t current = ABS(ADCSample.CurrentFilter);
    int32_t powerNow = (voltage * current) >> 9;

    // PI control: adjust the duty cycle so the actual power tracks the target power
    PICalc(&gPowerPI, gPowerTarget, powerNow);
}

// PowerLoopReset: power loop reset
void PowerLoopReset(void) {
    PIReset(&gPowerPI);
}

// PowerLoopSetPower: set the target power
void PowerLoopSetPower(float power) {
    if (power < 0) {
        power = 0;
    }
    float maxPower = POWER_MAX;
    if (power > maxPower) {
        power = maxPower;
    }
    gPowerTarget = _IQ15(power / POWER_MAX);
}

// PowerLoopGetPower: get the actual power
float PowerLoopGetPower(void) {
    int32_t voltage = ADCSample.Voltage;
    int32_t current = ABS(ADCSample.CurrentFilter);
    int32_t powerNow = (voltage * current) >> 9;
    return _IQ15toF(powerNow) * POWER_MAX;
}

// PowerLoopGetTarget: get the target power (IQ15 format)
float PowerLoopGetTarget(void) {
    return _IQ15toF(gPowerTarget) * POWER_MAX;
}

// PowerLoopGetOutput: get the power loop output
int32_t PowerLoopGetOutput(void) {
    return gPowerPI.Out;
}

// PowerLoopSetUI: set the power loop integral value
void PowerLoopSetUI(int32_t ui) {
    PISetUI(&gPowerPI, ui);
}

// PowerLoopIsSaturated: check whether the power loop is saturated
bool PowerLoopIsSaturated(void) {
    return gPowerPI.Out >= gPowerPI.OutMax && gPowerPI.Err > 0;
}

// PowerLoopSetOutMax: set the power loop maximum output
void PowerLoopSetOutMax(int32_t outMax) {
    gPowerPI.OutMax = outMax;
}

// PowerLoopSetOutMin: set the power loop minimum output
void PowerLoopSetOutMin(int32_t outMin) {
    gPowerPI.OutMin = outMin;
}

