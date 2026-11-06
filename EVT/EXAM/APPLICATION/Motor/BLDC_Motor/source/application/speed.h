/********************************** (C) COPYRIGHT  *******************************
 * File Name          : speed.h
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
#ifndef SPEED_H
#define SPEED_H

#include "framework.h"

// SpeedInit: initialization
void SpeedInit(void);

// SpeedRun: run
void SpeedRun(void);

// SpeedSetTime: set the time (sum of the commutation times of one electrical cycle)
// The internal normal path now uses the sliding window published by SpeedPushInterval(); this interface is only for special external use
void SpeedSetTime(uint32_t time);

// SpeedPushInterval: push one 60-degree commutation interval (called once per commutation)
// interval: SysTick counts
void SpeedPushInterval(uint32_t interval);

// SpeedResetWindow: clear only the speed feedback sliding window, leaving PI and the speed ramp untouched
void SpeedResetWindow(void);

// SpeedSet: set the target speed (with ramp-up)
void SpeedSet(uint32_t eRPM);

// SpeedSetDirect: set the speed directly (no ramp, effective immediately)
void SpeedSetDirect(uint32_t eRPM);

// SpeedRampUpdate: update the speed ramp (called internally)
void SpeedRampUpdate(void);

// SpeedSetParam: set the parameters
void SpeedSetParam(float kp, float ki, float outMax, float outMin);

// SpeedGet: get the speed
uint32_t SpeedGet(void);

// SpeedReset: reset
void SpeedReset(void);
// Main-loop reset of PI, filter and ramp, without modifying the commutation window held by the interrupt.
void SpeedResetControl(void);

// SpeedGetOutput: get the speed loop output
int32_t SpeedGetOutput(void);

// SpeedSetUI: set the speed loop integral value
void SpeedSetUI(int32_t value);

// SpeedIsSaturated: check whether the speed loop is saturated
bool SpeedIsSaturated(void);

// SpeedSetOutMax: set the speed loop maximum output
void SpeedSetOutMax(int32_t value);

// SpeedGetOutMax: get the speed loop maximum output
int32_t SpeedGetOutMax(void);

// SpeedSetOutMin: set the speed loop minimum output
void SpeedSetOutMin(int32_t value);

// SpeedGetOutMin: get the speed loop minimum output
int32_t SpeedGetOutMin(void);

// Main-loop only: read a consistent snapshot with limited retries, without disabling interrupts; keep the previous feedback on failure.
void SpeedCalc(void);

#endif
