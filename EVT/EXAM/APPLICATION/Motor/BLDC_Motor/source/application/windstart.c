/********************************** (C) COPYRIGHT  *******************************
 * File Name          : windstart.c
 * Author             : WCH
 * Version            : V1.0.1
 * Date               : 2026/11/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "windstart.h"
#include "adcsample.h"
#include "control.h"
#include "current.h"
#include "drvsystick.h"
#include "interface.h"
#include "powerloop.h"
#include "protect.h"
#include "pulse.h"
#include "pwm.h"
#include "speed.h"
#include "warmboot.h"

#ifndef WARMBOOT_REVERSE_TIMEOUT
#define WARMBOOT_REVERSE_TIMEOUT 1000
#endif
// Reverse-wind handling mode (compatible default: uses brake mode when the old project's config.h is not configured)
//   0: intermittent short-circuit brake + re-check, start as standstill after speed drops, keep the target direction
//   1: switch the rotation direction directly, enter closed loop synchronized to the detected direction (this run follows the wind; the target direction is restored after stop)
#ifndef WARMBOOT_REVERSE_MODE
#define WARMBOOT_REVERSE_MODE 0
#endif
// Short-circuit brake time per reverse-wind round (ms): the low-side circulating current does not flow through the bus shunt, so over-current protection cannot see it; determine it after measuring the phase current
#ifndef REVERSE_BRAKE_ON_MS
#define REVERSE_BRAKE_ON_MS 20
#endif
// Time to wait with all switches off for the winding freewheeling current to decay after braking (ms), then re-check BEMF
#ifndef REVERSE_DEMAG_MS
#define REVERSE_DEMAG_MS 2
#endif
// Time to keep the short circuit after BEMF disappears (ms), ensuring the rotor is at standstill before alignment
#ifndef REVERSE_HOLD_MS
#define REVERSE_HOLD_MS 200
#endif

static struct pt gReversePT = {0};
static bool gReverseActive = false;
static bool gReverseFinal = false;
static uint64_t gReverseStartMs = 0;
static bool gDirectionSwitched = false;
static uint32_t gTargetDirection = CW;

static void enterRun(void);
static void enterLocate(uint64_t *stateStartMs);
static void reverseEnter(bool final);

void WindStartReset(void) {
    WarmBootReset();
    PT_INIT(&gReversePT);
    gReverseActive = false;
    WindStartRestoreDirection();
}

void WindStartRestoreDirection(void) {
    if (gDirectionSwitched == true) {
        HoldParm.RotorDirection = gTargetDirection;
        gDirectionSwitched = false;
    }
}

void WindStartCheck(uint64_t *stateStartMs) {
    if (HoldParm.IsCmpMode == true) {
        // TODO must be handled in CMP mode: comparator mode does not yet support head/tail-wind detection, keep the original alignment path
#if STATIC_PULSE_INJECT_ENABLE == 1
        PulseReset();
        mcState = mcPulse;
        *stateStartMs = TZTimeGetMillsecond();
#else
        mcState = mcAlign;
#endif
        return;
    }

    // Head-wind detection
    if (WarmBootCheck() == true) {
        HoldParm.ZeroCrossTime = SYSTICK_CNT;
        MotorSpeedUpdate();
        DrvSystickClear();
    }

    // Consecutive valid commutations detected: distinguish head wind from tail wind
    if (WarmBootIsOK() == true) {
        if (WarmBootGetDirection() == HoldParm.RotorDirection) {
            // Head wind: enter closed loop
            enterRun();
            return;
        }
#if WARMBOOT_REVERSE_MODE == 1
        // Tail wind: switch the rotation direction directly and enter closed loop synchronized to the detected direction
        if (gDirectionSwitched == false) {
            gTargetDirection = HoldParm.RotorDirection;
            gDirectionSwitched = true;
        }
        HoldParm.RotorDirection = WarmBootGetDirection();
        enterRun();
#else
        // Tail wind: short-circuit brake, then return to mcAhead for a re-check
        reverseEnter(false);
#endif
    } else if (WarmBootGetIsBemf() == false || TZTimeGetMillsecond() - *stateStartMs > WARMBOOT_TIMEOUT) {
        if (gReverseActive == true) {
            // Tail wind has been braked until BEMF disappears (or is too low to measure): brake longer to ensure standstill, then start alignment
            reverseEnter(true);
            return;
        }
        enterLocate(stateStartMs);
    }
}

// Head wind (including tail wind after a direction switch): enter closed loop synchronized
// Before the call, HoldParm.RotorDirection must match WarmBootGetDirection()
static void enterRun(void) {
    gReverseActive = false;
    // Directly output the conducting phase where the zero crossing just occurred; run() then commutates after detecting the next zero crossing
    HoldParm.Phase = WarmBootGetSector();
    // Clear the speed/desync statistics and filter state (see MotorResetRunFeedback)
    MotorResetRunFeedback();
    // TODO this duty cycle should preferably come from a table indexed by motor speed
    MotorSetStartupDuty(PWM_DUTYCYCLE_25);
    // Bumpless transfer: preset the loop integrator (same as mcDrag->mcRun), otherwise the integrator stays at its reset value
    // and the very first control cycle rewrites the duty cycle to a value unrelated to the entry duty
    PWMApplyPhase();
    SYSTICK_CLEAR();
    MotorEnterRun();
}

// enterLocate: when no BEMF is detected (or it is gone), enter static pulse injection alignment (if enabled) or conventional alignment
static void enterLocate(uint64_t *stateStartMs) {
#if STATIC_PULSE_INJECT_ENABLE == 1
    PulseReset();
    mcState = mcPulse;
    *stateStartMs = TZTimeGetMillsecond();
#else
    (void)stateStartMs;
    mcState = mcAlign;
#endif
    SYSTICK_CLEAR();
    HoldParm.SpeedTime_Sum = 0;
}

// reverseEnter: enter the reverse-wind brake
// final: false means intermittent braking (return to mcAhead for a re-check after braking); true means the final holding brake after BEMF disappears (go to alignment after braking)
static void reverseEnter(bool final) {
    uint64_t now = TZTimeGetMillsecond();
    if (gReverseActive == false) {
        gReverseActive = true;
        gReverseStartMs = now;
    } else if (final == false && now - gReverseStartMs > WARMBOOT_REVERSE_TIMEOUT) {
        // Reverse wind too strong to brake: report stall and stop
        ProtectSetError(ERROR_MASK_STALL);
        gMotorIsNeedRun = false;
        return;
    }
    gReverseFinal = final;
    PT_INIT(&gReversePT);
    mcState = mcReverse;
}

// Reverse-wind brake: low-side short-circuit brake -> all off waiting for demagnetization -> return to mcAhead for a re-check or enter alignment
int WindStartReverse(uint64_t *stateStartMs) {
    static uint64_t time = 0;

    PT_BEGIN(&gReversePT);

    HoldParm.PhaseIsChange = true; // Suppress the SysTick fallback commutation
    PWMSetLonHoff();
    time = TZTimeGetMillsecond();
    PT_WAIT_UNTIL(&gReversePT, TZTimeGetMillsecond() - time >= (gReverseFinal ? REVERSE_HOLD_MS : REVERSE_BRAKE_ON_MS));

    PWMSetAllOff();
    time = TZTimeGetMillsecond();
    PT_WAIT_UNTIL(&gReversePT, TZTimeGetMillsecond() - time >= REVERSE_DEMAG_MS);

    HoldParm.PhaseIsChange = false;
    if (gReverseFinal == true) {
        gReverseActive = false;
        enterLocate(stateStartMs);
    } else {
        // Return to head/tail-wind detection: switch state first, then reset the detection; the injected sequence is fixed so no channel switch is needed
        WarmBootReset();
        mcState = mcAhead;
        SYSTICK_CLEAR();
        *stateStartMs = TZTimeGetMillsecond();
    }

    PT_END(&gReversePT);
}
