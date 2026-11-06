/********************************** (C) COPYRIGHT  *******************************
 * File Name          : speed.c
 * Author             : WCH
 * Version            : V1.0.1
 * Date               : 2026/11/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "speed.h"
#include "config.h"
#include "isr_sync.h"

// Sum of the commutation times of one electrical cycle (6 x 60-degree intervals); written by the ISR, read by the main loop, so it must be volatile
static volatile uint32_t gSpeedTimeSum = 0;
static volatile uint32_t gSpeedFrame = 0;
static uint32_t gConsumedFrame = UINT32_MAX;
static int32_t gRpmFiltered = 0; // RPM after IIR filtering

// Speed feedback sliding window
// The window width is fixed at 6 x 60-degree intervals (= 1 electrical cycle), preserving cancellation of sector asymmetry (6th harmonic);
// but it is published on every commutation, reducing the feedback ZOH delay from 3T to 0.5T, and the IIR below
// also updates at the commutation rate instead of 1/6 of it, cutting the equivalent time constant by another factor of 6
#define SPEED_WIN_LEN 6
static uint32_t gWin[SPEED_WIN_LEN] = {0};
static uint32_t gWinSum = 0;
static uint8_t gWinIdx = 0;
static uint8_t gWinFill = 0;

// Speed first-order IIR coefficient. The sliding window already averages a full electrical cycle, so this only suppresses zero-crossing jitter.
// Group delay = (1-a)/a update periods, where one update period = 1 commutation:
//   0.1 -> 9 commutations   0.25 -> 3   0.4 -> 1.5
#ifndef SPEED_IIR_ALPHA
#define SPEED_IIR_ALPHA 0.1f
#endif

// Speed ramp control
static int32_t gTargetSpeed = 0;     // Target speed (Q15 format)
static int32_t gCurrentSetSpeed = 0; // Current set speed (Q15 format)
static uint64_t gLastRampTime = 0;   // Last ramp update time

#ifndef SPEED_RAMP_RATE
#define SPEED_RAMP_RATE 100.0f // Default slope 100 eRPM/ms
#endif

// Compile-time precomputed constants to eliminate run-time floating point
// SPEED_CALC_NUMERATOR is computed at compile time as uint64 and narrowed after division:
// with a high core clock and a small SPEED_MAX this numerator exceeds int32 (e.g. 144MHz + SPEED_MAX=100000 -> 2.83e9)
#define SPEED_RAMP_Q15_PER_MS ((int32_t)(SPEED_RAMP_RATE * 32768.0 / (double)SPEED_MAX))
#define SPEED_CALC_NUMERATOR ((uint64_t)SYSTEM_CLOCK_FREQ * 60u * 32768u / SPEED_MAX)

// Fail the build when the ramp step is truncated to 0 (otherwise the ramp freezes without any hint)
_Static_assert(SPEED_RAMP_Q15_PER_MS > 0,
               "SPEED_RAMP_Q15_PER_MS truncated to 0: SPEED_RAMP_RATE too small vs SPEED_MAX");

static PIParam gPIParam;

// Writers are mutually exclusive between ADC and CMP modes; external resets are still serialized by a short critical section; an even sequence number means a complete frame.
__attribute__((always_inline)) static inline void publishSpeed(uint32_t sum) {
    uint32_t frame = IsrLoadRelaxed(&gSpeedFrame);
    IsrStoreRelaxed(&gSpeedFrame, frame + 1u);
    IsrReleaseFence();
    IsrStoreRelaxed(&gSpeedTimeSum, sum);
    IsrStoreRelease(&gSpeedFrame, frame + 2u);
}

// SpeedInit: initialization
void SpeedInit(void) {
    PISetParam(&gPIParam, SPEED_KP, SPEED_KI, SPEED_OUT_MAX, SPEED_OUT_MIN);
    gTargetSpeed = 0;
    gCurrentSetSpeed = 0;
    gLastRampTime = 0;
}

// SpeedRun: run
void SpeedRun(void) {
    // Update the speed ramp
    SpeedRampUpdate();
    // Speed loop output
    PICalc(&gPIParam, HoldParm.SetSpeed, HoldParm.SpeedFilter);
}

// SpeedCalc: calculate the speed
void SpeedCalc(void) {
    uint32_t frame = 0, sum = 0;
    bool complete = false;
    // Main-loop reads only; on limited-retry failure keep the old feedback and never let the interrupt wait for a preempted writer.
    for (uint8_t retry = 0; retry < 3; retry++) {
        frame = IsrLoadAcquire(&gSpeedFrame);
        if ((frame & 1u) != 0) {
            return;
        }
        sum = IsrLoadRelaxed(&gSpeedTimeSum);
        IsrAcquireFence();
        if (frame == IsrLoadRelaxed(&gSpeedFrame)) {
            complete = true;
            break;
        }
    }
    if (!complete) {
        return;
    }
    if (sum == 0 || frame == gConsumedFrame) {
        return;
    }
    gConsumedFrame = frame;

    int32_t rawRpm = (int32_t)(SPEED_CALC_NUMERATOR / sum);
    if (gRpmFiltered == 0) {
        gRpmFiltered = rawRpm; // Assign directly on the first sample
    } else {
        gRpmFiltered += _IQ15mpy(_IQ15(SPEED_IIR_ALPHA), rawRpm - gRpmFiltered);
    }
    HoldParm.SpeedFilter = gRpmFiltered;
    HoldParm.Speed = rawRpm;
}

// SpeedPushInterval: push one 60-degree commutation interval (called from zero-crossing handling, once per commutation)
// interval: SysTick counts
FAST_FUNC void SpeedPushInterval(uint32_t interval) {
    if (gWinFill < SPEED_WIN_LEN) {
        gWinSum += interval;
        gWin[gWinIdx] = interval;
        gWinFill++;
    } else {
        gWinSum += interval - gWin[gWinIdx];
        gWin[gWinIdx] = interval;
    }

    if (++gWinIdx >= SPEED_WIN_LEN) {
        gWinIdx = 0;
    }

    // Do not output until the window is full: keeps the pre-change behavior where the first speed value only appears after 6 intervals
    if (gWinFill >= SPEED_WIN_LEN) {
        publishSpeed(gWinSum);
    }
}

// SpeedResetWindow: clear only the speed feedback sliding window, leaving PI and the ramp untouched
// Called on the mcDrag -> mcRun transition so that the slow commutation intervals of the drag phase are not carried into closed loop
void SpeedResetWindow(void) {
    uint32_t irqSave = criticalEnter();
    // The old slots are not read until the window fills again, so the whole array does not need clearing with interrupts disabled.
    gWinSum = 0;
    gWinIdx = 0;
    gWinFill = 0;
    publishSpeed(0);
    criticalExit(irqSave);
}

void SpeedSetParam(float kp, float ki, float outMax, float outMin) {
    PISetParam(&gPIParam, kp, ki, outMax, outMin);
}

// SpeedSet: set the target speed (with ramp)
void SpeedSet(uint32_t eRPM) {
    if (eRPM < SPEED_MIN) {
        eRPM = SPEED_MIN;
    } else if (eRPM > SPEED_MAX) {
        eRPM = SPEED_MAX;
    }
    int32_t newTarget = (int32_t)((uint64_t)eRPM * 32768 / SPEED_MAX);
    if (newTarget == gTargetSpeed && gLastRampTime != 0) {
        return;
    }
    gTargetSpeed = newTarget;

    // On the first call, use the current actual speed as the starting point
    if (gLastRampTime == 0) {
        gCurrentSetSpeed = HoldParm.SpeedFilter; // Use the current actual speed
        HoldParm.SetSpeed = gCurrentSetSpeed;
        gLastRampTime = TZTimeGetMillsecond();
    }
}

// SpeedSetDirect: set the speed directly (no ramp)
void SpeedSetDirect(uint32_t eRPM) {
    if (eRPM < SPEED_MIN) {
        eRPM = SPEED_MIN;
    } else if (eRPM > SPEED_MAX) {
        eRPM = SPEED_MAX;
    }
    gTargetSpeed = (int32_t)((uint64_t)eRPM * 32768 / SPEED_MAX);
    gCurrentSetSpeed = gTargetSpeed;
    HoldParm.SetSpeed = gCurrentSetSpeed;
    gLastRampTime = TZTimeGetMillsecond();
}

// SpeedRampUpdate: update the speed ramp
void SpeedRampUpdate(void) {
    if (gLastRampTime == 0) {
        if (gTargetSpeed == 0) {
            return; // Target speed not set yet, do not update
        }
        // A target speed exists but ramp is not initialized: start ramping from the current actual speed
        gCurrentSetSpeed = HoldParm.SpeedFilter;
        HoldParm.SetSpeed = gCurrentSetSpeed;
        gLastRampTime = TZTimeGetMillsecond();
        return;
    }

    uint64_t currentTime = TZTimeGetMillsecond();
    uint64_t deltaTime = currentTime - gLastRampTime;

    if (deltaTime == 0) {
        return; // Time has not changed, do not update
    }

    if (deltaTime > 1000) {
        deltaTime = 1000; // Clamp to prevent overflow
    }

    gLastRampTime = currentTime;

    // Calculate the maximum allowed change (from slope and time, integer arithmetic only)
    int32_t maxChange = (int32_t)deltaTime * SPEED_RAMP_Q15_PER_MS;

    int32_t speedDiff = gTargetSpeed - gCurrentSetSpeed;

    if (speedDiff > maxChange) {
        // Need to accelerate but limited by the slope
        gCurrentSetSpeed += maxChange;
    } else if (speedDiff < -maxChange) {
        // Need to decelerate but limited by the slope
        gCurrentSetSpeed -= maxChange;
    } else {
        // The difference is within the allowed range, reach the target directly
        gCurrentSetSpeed = gTargetSpeed;
    }

    HoldParm.SetSpeed = gCurrentSetSpeed;
}

// SpeedGet: get the speed
uint32_t SpeedGet(void) {
    return _IQ15toF(HoldParm.SetSpeed) * SPEED_MAX;
}

// SpeedReset: reset
void SpeedReset(void) {
    SpeedResetWindow();
    SpeedResetControl();
    uint32_t irqSave = criticalEnter();
    HoldParm.SpeedTime_Sum = 0;
    HoldParm.SpeedTime_Cnt = 0;
    criticalExit(irqSave);
}

void SpeedResetControl(void) {
    gConsumedFrame = UINT32_MAX;
    gRpmFiltered = 0;
    HoldParm.SpeedFilter = 0;
    HoldParm.Speed = 0;
    // gTargetSpeed = 0; // Keep the target speed, it survives a reset
    gCurrentSetSpeed = 0;
    gLastRampTime = 0;
    PIReset(&gPIParam);
}

// SpeedGetOutput: get the speed loop output
int32_t SpeedGetOutput(void) {
    return gPIParam.Out;
}

// SpeedSetUI: set the speed loop integral value
void SpeedSetUI(int32_t value) {
    PISetUI(&gPIParam, value);
}

// SpeedSetOutMax: set the speed loop maximum output
void SpeedSetOutMax(int32_t value) {
    gPIParam.OutMax = value;
}

// SpeedGetOutMax: get the speed loop maximum output
int32_t SpeedGetOutMax(void) {
    return gPIParam.OutMax;
}

// SpeedSetOutMin: set the speed loop minimum output
void SpeedSetOutMin(int32_t value) {
    gPIParam.OutMin = value;
}

// SpeedGetOutMin: get the speed loop minimum output
int32_t SpeedGetOutMin(void) {
    return gPIParam.OutMin;
}

// SpeedIsSaturated: check whether the speed loop is saturated
bool SpeedIsSaturated(void) {
    return gPIParam.Out >= gPIParam.OutMax && gPIParam.Err > 0;
}
