/********************************** (C) COPYRIGHT  *******************************
 * File Name          : current.c
 * Author             : WCH
 * Version            : V1.0.1
 * Date               : 2026/11/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "current.h"
#include "config.h"

// Guarded defaults for the current limit loop parameters
#ifndef CURRENT_KP
#define CURRENT_KP 0.5f
#endif
#ifndef CURRENT_KI
#define CURRENT_KI 0.1f
#endif
#ifndef CURRENT_OUT_MAX
#define CURRENT_OUT_MAX 1.0f
#endif
#ifndef CURRENT_OUT_MIN
#define CURRENT_OUT_MIN 0.0f
#endif
#ifndef CURRENT_LIMIT
#define CURRENT_LIMIT 100.0f
#endif
#ifndef CURRENT_MAX
#define CURRENT_MAX 660.0f  // Default maximum current 660A
#endif

static PIParam gCurrentPI;
static int32_t gCurrentLimit = 0;
static bool gIsLimiting = false;

// CurrentInit: current limit loop initialization
void CurrentInit(void) {
    gCurrentPI.Mode = PI_MODE_PARALLEL;
    PISetParam(&gCurrentPI, CURRENT_KP, CURRENT_KI, CURRENT_OUT_MAX, CURRENT_OUT_MIN);
    gCurrentLimit = _IQ15(CURRENT_LIMIT / CURRENT_MAX);
}

// CurrentRun: current limit loop run
void CurrentRun(void) {
    // Get the current now (absolute value after filtering)
    int32_t currentNow = ABS((int32_t)ADCSample.CurrentFilter) << 3;

    // PI runs continuously: in cascade mode the output is only taken while limiting (below the limit the integrator is overwritten by CurrentSetUI),
    // while in current-only mode it regulates constant current directly - below the limit the output saturates at OutMax to push the duty cycle up faster
    gIsLimiting = (currentNow >= gCurrentLimit);
    PICalc(&gCurrentPI, gCurrentLimit, currentNow);
}

// CurrentReset: current limit loop reset
void CurrentReset(void) {
    PIReset(&gCurrentPI);
    gIsLimiting = false;
}

// CurrentSetLimit: set the current limit
void CurrentSetLimit(float current) {
    if (current < 0) {
        current = 0;
    } else if (current > CURRENT_MAX) {
        current = CURRENT_MAX;
    }
    gCurrentLimit = _IQ15(current / CURRENT_MAX);
}

// CurrentGetLimit: get the current limit
float CurrentGetLimit(void) {
    return _IQ15toF(gCurrentLimit) * CURRENT_MAX;
}

// CurrentGetOutput: get the current limit loop output
int32_t CurrentGetOutput(void) {
    return gCurrentPI.Out;
}

// CurrentIsLimiting: check whether limiting is active
bool CurrentIsLimiting(void) {
    return gIsLimiting;
}

// CurrentIsSaturated: check whether the current limit loop output is saturated (duty cycle at the upper limit while current is still below the limit)
bool CurrentIsSaturated(void) {
    return gCurrentPI.Out >= gCurrentPI.OutMax && gCurrentPI.Err > 0;
}

// CurrentSetUI: set the current loop integral value
void CurrentSetUI(int32_t ui) {
    PISetUI(&gCurrentPI, ui);
}

// CurrentSetOutMax: set the current loop maximum output
void CurrentSetOutMax(int32_t outMax) {
    gCurrentPI.OutMax = outMax;
}

// CurrentSetOutMin: set the current loop minimum output
void CurrentSetOutMin(int32_t outMin) {
    gCurrentPI.OutMin = outMin;
}
