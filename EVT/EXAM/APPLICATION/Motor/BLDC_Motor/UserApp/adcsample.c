/********************************** (C) COPYRIGHT  *******************************
 * File Name          : adcsample.c
 * Author             : WCH
 * Version            : V1.0.1
 * Date               : 2026/11/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "adcsample.h"
#include "framework.h"
#include "control.h"

// Current first-order IIR filter coefficient (main loop tick)
#define IIR_ALPHA 0.05f

// AdcSampleLoop: ADC cyclic sampling of voltage and current
void AdcSampleLoop(void) {
    static bool invalidActive;
    static uint64_t invalidSinceUs;
    DrvAdcRegularSample_T regular;
    if (DrvAdcReadRegular(&regular) == false) {
        if (!invalidActive) {
            invalidActive = true;
            invalidSinceUs = TZTimeGet();
        } else if (TZTimeGet() - invalidSinceUs > 200u) {
            MotorEmergencyStop(ERROR_MASK_SAMPLE_TIMEOUT);
        }
        return;
    }
    invalidActive = false;
    ADCSample.Voltage = regular.voltage;
    // No external RC-filtered IBUS channel on this board: the average current is
    // derived by first-order IIR filtering the injected-group OPA instantaneous
    // current (zero offset already removed in control.c) in the main loop, and is
    // used by the over-current protection and the current limit loop.
    if (ADCSample.CalibrationIsOK == true) {
        ADCSample.CurrentFilter += _IQ15mpy(_IQ15(IIR_ALPHA), (ADCSample.Current - ADCSample.CurrentFilter));
    }
}
