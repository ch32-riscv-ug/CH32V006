/********************************** (C) COPYRIGHT  *******************************
 * File Name          : startup.h
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
#ifndef STARTUP_H
#define STARTUP_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Startup configuration parameters (loaded at run time, replacing the config.h compile-time macros)
typedef struct {
    uint8_t  zeroMaxCnt;              // Number of zero-crossing detections at startup
    uint32_t systemClockFreq;         // System clock frequency (Hz)
    uint32_t speedMin;                // Minimum speed (eRPM)
    uint32_t phaseDelayEnable;        // Commutation delay enable (0/1)
    float    dragVoltageMin;          // Minimum forced-drag voltage (V)
    float    dragVoltageMax;          // Maximum forced-drag voltage (V)
    float    voltageMax;              // Maximum supported voltage (V)
    uint32_t stallTimeMs;             // Stall protection time (ms)
    uint32_t avoidChargingTimeUs;     // Freewheeling avoidance time (us)
    int32_t  tempAdvance;             // Commutation advance angle during startup (Q15 per-unit)
    uint32_t dragRampTimeMs;          // Drag ramp-up time: linear time for the duty cycle to rise from dutyMin to dutyMax (ms)
} StartupConfig_T;

// StartupLoad: load
// config: configuration parameter pointer; pass NULL to use the defaults (all 0, not recommended)
void StartupLoad(const StartupConfig_T *config);

// startupCheckZeroCross: detect zero crossings
void StartupRun(void);

// StartupReset: reset the startup parameters
void StartupReset(void);

// StartupIsFinish: check whether startup has finished
bool StartupIsFinish(void);

// StartupGetDutyMin: get the minimum duty cycle at startup
int32_t StartupGetDutyMin(void);

// StartupGetDutyMax: get the maximum duty cycle at startup
int32_t StartupGetDutyMax(void);

// StartupGetZeroCrossCount: get the zero-crossing count at startup
uint8_t StartupGetZeroCrossCount(void);

#endif
