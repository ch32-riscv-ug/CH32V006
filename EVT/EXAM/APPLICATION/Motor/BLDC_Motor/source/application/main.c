/********************************** (C) COPYRIGHT  *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.0.1
 * Date               : 2026/11/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "framework.h"
#include "control.h"
#include "current.h"
#include "isr.h"
#include "launch.h"
#include "pulse.h"
#include "speed.h"
#include "startup.h"
#include "warmboot.h"
#include "powerloop.h"

// Drag ramp-up time (ms): time for the duty cycle to rise linearly from DRAG_VOLTAGE_MIN to DRAG_VOLTAGE_MAX.
// When not defined in config.h, 200ms is used - equivalent to the actual ramp duration of the old interrupt-count based scheme:
// old step = +1 LSB every 6 ADC interrupts; during drag the duty cycle is low so only 1 sample point fits => 20kHz => 300us/step,
// t_ramp = 32768 * (DRAG_VOLTAGE_MAX - DRAG_VOLTAGE_MIN) / Vbus * 300us = 9.83s * dV / Vbus
#ifndef DRAG_RAMP_TIME
#define DRAG_RAMP_TIME 200
#endif

int main() {
    FrameworkLoad();
    TZTimeLoad(SysTimeGet);
    TZTimeEnableFast(true);
    SpeedInit();
    CurrentInit();
    PowerLoopInit();

    static const StartupConfig_T startupCfg = {
        .zeroMaxCnt              = STARTUP_ZCP_CNT,
        .systemClockFreq         = SYSTEM_CLOCK_FREQ,
        .speedMin                = SPEED_MIN,
        .phaseDelayEnable        = PHASE_DELAY_ENABLE,
        .dragVoltageMin          = DRAG_VOLTAGE_MIN,
        .dragVoltageMax          = DRAG_VOLTAGE_MAX,
        .voltageMax              = VOLTAGE_MAX,
        .stallTimeMs             = STALL_TIME,
        .avoidChargingTimeUs     = AVOID_CHARGING_TIME,
        .tempAdvance             = _IQ15(0.0),
        .dragRampTimeMs          = DRAG_RAMP_TIME,
    };
    StartupLoad(&startupCfg);

    WarmBootInit(WARMBOOT_VOLTAGE_THRESHOLD);

    static const PulseConfig_T pulseCfg = {
        .systemClockFreq     = SYSTEM_CLOCK_FREQ,
        .adcTimeoutUs        = STATIC_PULSE_ADC_TIMEOUT_US,
        .demagTimeUs         = STATIC_PULSE_DEMAG_TIME_US,
        .settleTimeUs        = STATIC_PULSE_SETTLE_TIME_US,
        .injectDuty          = STATIC_PULSE_INJECT_DUTY,
        .phaseLossThreshold  = STATIC_PULSE_PHASE_LOSS_THRESHOLD,
        .shortThreshold      = STATIC_PULSE_SHORT_THRESHOLD,
        .adcPause            = DrvAdcPauseRegular,
        .adcResume           = DrvAdcResumeRegular,
    };
    PulseLoad(&pulseCfg);

    LaunchLoad();
    while (1) {
        LaunchRun();
        FrameworkRun();
        MotorWhile();
    }
}
