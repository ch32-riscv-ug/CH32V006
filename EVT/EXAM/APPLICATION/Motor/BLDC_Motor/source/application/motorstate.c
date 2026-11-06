/********************************** (C) COPYRIGHT  *******************************
 * File Name          : motorstate.c
 * Author             : WCH
 * Version            : V1.0.1
 * Date               : 2026/11/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "motorstate.h"
#include "adcsample.h"
#include "control.h"
#if CMP_SUPPORT_ENABLE == 1
#include "cmp.h"
#include "drvcmp.h"
#endif
#include "comparator.h"
#include "critical.h"
#include "current.h"
#include "drvsystick.h"
#include "halless.h"
#include "interface.h"
#include "isr_sync.h"
#include "powerloop.h"
#include "protect.h"
#include "pulse.h"
#include "pwm.h"
#include "speed.h"
#include "startup.h"
#include "windstart.h"

volatile uint32_t gMotorIsNeedRun = false;

static struct pt gAlignPT = {0};
static struct pt gBrakePT = {0};
static struct pt gChargePT = {0};

static uint64_t gStartTime = 0;
static uint32_t gBrakeTimeoutMs = 0; // Maximum brake duration (ms), set only by the ADC state machine
static uint64_t gBrakeStartMs = 0;   // Brake start timestamp (ms)

typedef struct {
    uint32_t stopEpoch;
    uint32_t brakeTimeMs; // 0 means start, non-zero means normal brake; emergency stop bypasses the mailbox.
    uint32_t acknowledgedErrors;
} MotorRequest_T;

// Published only by the main loop, consumed only by the ADC; only the latest target is kept, so short key jitter is not queued as pending actions.
static MotorRequest_T gMotorRequests[2];
static uint32_t gMotorRequestToken = 0;
static uint32_t gMotorRequestConsumed = 0;
static volatile uint32_t gEmergencyEpoch = 0;

static void idle(void);
static void pulse(void);
static int alignTask(void);
static int charge(void);
static void drag(void);
static int brake(void);
static inline void adcCalibration(void);

// MotorInit: initialization
void MotorInit(void) {
    // Turn off the outputs and clear the previous compare values first, so the first drag/head-wind entry cannot briefly reuse the old high duty cycle.
    uint32_t outputSave = criticalEnter();
    TIM_CtrlPWMOutputs(TIM1, DISABLE);
    PWMSetAllOff();
    TIM1->CH1CVR = 0;
    TIM1->CH2CVR = 0;
    TIM1->CH3CVR = 0;
    criticalExit(outputSave);
    PWMSetFrequency(PWM_FRE);
    ParamReset();
    SpeedResetWindow();
    StartupReset();
    HallessReset();
    PulseReset();
    PT_INIT(&gAlignPT);
    PT_INIT(&gBrakePT);
    PT_INIT(&gChargePT);
    // Restarting right after braking skips mcStop, so the target direction still has to be restored.
    WindStartReset();
    MotorResetControl(true);
    SYSTICK_CLEAR();
    // Every startup (cold start/hot restart share this one path: mcIdle->MotorInit()->mcCharge->mcAhead) resets
    // CMP to "never match". During mcAhead (head/tail-wind detection) nobody loads CMP actively; if the commutation delay
    // left by delayCharge() from the previous run were kept (usually far smaller than WARMBOOT_TIMEOUT), SysTick_Handler
    // would trigger PWMSwitchPhase() prematurely and force the phase under test low, corrupting the three-phase BEMF
    // readings of head-wind detection. The commutation delay and stall fallback in the mcRun stage both re-arm CMP explicitly and do not rely on this value.
    SYSTICK_CMP = 0xFFFFFFFF;
    MotorAdcSamplingReset();
    gStartTime = TZTimeGetMillsecond();

#if CMP_SUPPORT_ENABLE == 1
    if (HoldParm.SensorType == CMP_SENSOR) {
        ComparatorReset();
        ComparatorPollingStop();
        DrvCmpITCmd(false);
    }
#endif
    // Initialization may be preempted by an emergency stop, so the main outputs are re-enabled only if the request is still valid.
    uint32_t irqSave = criticalEnter();
    if (gMotorIsNeedRun && (ErrorCode.value & ERROR_MASK_BLOCKING) == 0) {
        TIM_CtrlPWMOutputs(TIM1, ENABLE);
    }
    criticalExit(irqSave);
}

// MotorStateRun: non-run state machine, called by MotorControl while not in mcRun
void MotorStateRun(void) {
    switch (mcState) {
    case mcIdle:
        idle();
        break;
    case mcCharge:
        charge();
        break;
    case mcAhead:
        WindStartCheck(&gStartTime);
        break;
    case mcPulse:
        pulse();
        break;
    case mcAlign:
        alignTask();
        break;
    case mcDrag:
        drag();
        break;
    case mcdec:
        brake();
        break;
    case mcReverse:
        WindStartReverse(&gStartTime);
        break;
    case mcStop:
        mcState = mcIdle;
        PT_INIT(&gBrakePT);
        WindStartRestoreDirection();
        DrvSystickCmd(false);
        ParamReset();
#if CMP_SUPPORT_ENABLE == 1
        DrvCmpITCmd(false);
#endif
        break;
    default:
        break;
    }
}

static void idle(void) {
    if (ADCSample.CalibrationIsOK == false) {
        adcCalibration();
        return;
    }


    if (gMotorIsNeedRun == true) {
        if ((ErrorCode.value & ERROR_MASK_BLOCKING) != 0) {
            gMotorIsNeedRun = false;
            return;
        }
        MotorInit();
        uint32_t irqSave = criticalEnter();
        if (gMotorIsNeedRun && (ErrorCode.value & ERROR_MASK_BLOCKING) == 0) {
            mcState = mcCharge;
        }
        criticalExit(irqSave);
    }
}

// ADC calibration
static inline void adcCalibration(void) {
#ifdef ADC_CURRENT_CALIBRATION_CALLBACK
    ADC_CURRENT_CALIBRATION_CALLBACK();
#else
    if (ADCSample.CalibrationIsOK == true) {
        return;
    }
    // Average over 64 calibration samples
    if (++ADCSample.CalibrationCnt > 64) {
        ADCSample.CurrentOffset = ADCSample.CalibrationSum >> 6;
        ADCSample.CalibrationSum = 0;
        ADCSample.CalibrationCnt = 0;
        ADCSample.CalibrationIsOK = true;
    } else {
        // Current is now the injected raw value minus the zero offset; calibration must accumulate raw values, so add the old offset back
        ADCSample.CalibrationSum += (int32_t)ADCSample.Current + (int32_t)ADCSample.CurrentOffset;
    }
#endif
}

// Static pulse injection rotor position detection
static void pulse(void) {
    if (PulseDetect() == true) {
        // Detection complete, enter forced-drag startup
        gStartTime = TZTimeGetMillsecond();
        SYSTICK_CLEAR();
        PulseReset();
        mcState = mcDrag;
        // Directly output the initial phase found by static pulse detection (no switching); the subsequent voltage boost and stall commutation are handled by drag()/boost()
        PWMApplyPhase();
        if (HoldParm.SensorType == CMP_SENSOR) {
            ComparatorPollingStart();
        }
    } else {
        // Detection failed (ADC sampling timeout, phase loss or short circuit): report a fault and stop
        // Phase loss/short circuit are already set inside PulseDetect; only other failures (ADC timeout, etc.) are handled here
        if (ErrorCode.bit.PhaseLoss == 0 && ErrorCode.bit.PhaseShort == 0) {
            ProtectSetError(ERROR_MASK_STALL);
        }
        MotorEmergencyStop(0);
    }
}

// Motor forced alignment task
static int alignTask(void) {
    static uint64_t time = 0;
    static int i = 0;

    PT_BEGIN(&gAlignPT);

    // Force the motor into alignment
    MotorSetStartupDuty(ALIGN_PWM_DUTY);
    for (i = 0; i < ALIGN_CNT; i++) {
        PWMSwitchPhase();
        time = TZTimeGetMillsecond();
        PT_WAIT_UNTIL(&gAlignPT, TZTimeGetMillsecond() - time > ALIGN_TIME);
    }

    uint32_t stateSave = criticalEnter();
    mcState = (gMotorIsNeedRun && (ErrorCode.value & ERROR_MASK_BLOCKING) == 0) ? mcDrag : mcStop;
    criticalExit(stateSave);
    if (HoldParm.SensorType == CMP_SENSOR) {
        ComparatorPollingStart();
    }
    gStartTime = TZTimeGetMillsecond();
    SYSTICK_CLEAR();

    PT_END(&gAlignPT);
}

static int charge(void) {
    static uint64_t time = 0;
    static uint8_t i = 0;
    PT_BEGIN(&gChargePT);

    // Low-side precharge time
    for (i = 0; i < 3; i++) {
        PWMCharge(i + 1);
        time = TZTimeGet();
        PT_WAIT_UNTIL(&gChargePT, TZTimeGet() - time > LOW_BRIDGE_CHARGE_TIME);
    }

    PWMSetAllOff();
    mcState = mcAhead;
    DrvSystickCmd(true);
    gStartTime = TZTimeGetMillsecond();
    i = 0;

    PT_END(&gChargePT);
}

static void drag(void) {
    StartupRun();
    if (StartupIsFinish() == true) {
        // Clear the speed/desync statistics so the slow commutation intervals of the drag phase are not carried into closed loop (see MotorResetRunFeedback)
        MotorResetRunFeedback();
#if CMP_SUPPORT_ENABLE == 1
        if (HoldParm.SensorType == ADC_CMP_SENSOR) {
            // Do not force a switch to CMP; run() switches automatically via cmpAdcSwitch() according to speed
            HoldParm.IsCmpMode = false;
        } else if (HoldParm.SensorType == CMP_SENSOR) {
            ComparatorPollingStop();
        }
        if (HoldParm.IsCmpMode == true) {
            CmpChannelCharge();
            DrvCmpITCmd(true);
        }
#endif
        MotorEnterRun();
        return;
    }

    if (TZTimeGetMillsecond() - gStartTime > START_TIMEOUT || gMotorIsNeedRun == false) {
        MotorEmergencyStop(gMotorIsNeedRun ? ERROR_MASK_STALL : 0);
    }
}

// brakeStep: brake exit conditions:
//   1) Brake time elapsed: gBrakeTimeoutMs milliseconds have passed
//   2) User restarts: gMotorIsNeedRun == true (MotorStart was called)
static bool brakeStep(void) {
    uint64_t elapsed = TZTimeGetMillsecond() - gBrakeStartMs;
    return (elapsed >= gBrakeTimeoutMs || gMotorIsNeedRun == true);
}

// brake: dissipative braking, all high-side off + three-phase low-side hard short (see the MotorStop note in control.h)
static int brake(void) {
    PT_BEGIN(&gBrakePT);

    // Fixed low-side short-circuit brake
    HoldParm.PhaseIsChange = true; // Suppress the SysTick fallback commutation
#if CMP_SUPPORT_ENABLE == 1
    DrvCmpITCmd(false);
#endif
    PWMSetLonHoff();
    gBrakeStartMs = TZTimeGetMillsecond();

    // Wait for the exit condition: time elapsed or user restart
    PT_WAIT_UNTIL(&gBrakePT, brakeStep());

    PWMSetAllOff();
    // If gMotorIsNeedRun is true (user restarted early) go back to mcIdle, otherwise transition explicitly to mcStop
    mcState = (gMotorIsNeedRun == false) ? mcStop : mcIdle;

    PT_END(&gBrakePT);
}

static void publishMotorRequest(uint32_t brakeTimeMs) {
    uint32_t stopEpoch = IsrLoadAcquire(&gEmergencyEpoch);
    uint32_t token = IsrLoadRelaxed(&gMotorRequestToken) + 1u;
    MotorRequest_T *request = &gMotorRequests[token & 1u];
    request->stopEpoch = stopEpoch;
    request->brakeTimeMs = brakeTimeMs;
    request->acknowledgedErrors = ErrorCode.value;
    IsrStoreRelease(&gMotorRequestToken, token);
}

// Normal start/stop state changes are centralized in the ADC context; emergency stop only keeps an indivisible safety commit.
FAST_FUNC void MotorStateProcessRequests(void) {
    uint32_t token = IsrLoadAcquire(&gMotorRequestToken);
    if (token == gMotorRequestConsumed) {
        return;
    }
    MotorRequest_T request = gMotorRequests[token & 1u];
    gMotorRequestConsumed = token;
    bool stopNow = false;
    uint32_t irqSave = criticalEnter();
    if (request.stopEpoch == gEmergencyEpoch) {
        if (request.brakeTimeMs == 0) {
            uint16_t persistent = ERROR_MASK_OVER_VOLTAGE | ERROR_MASK_UNDER_VOLTAGE | ERROR_MASK_TEMP_OVER;
            uint16_t clearMask = (uint16_t)request.acknowledgedErrors & (uint16_t)~persistent;
            // A new request is an explicit acknowledgment; voltage/temperature faults must still meet their recovery conditions first.
            // Only faults that already existed when the request was published are acknowledged, so faults appearing while queued cannot be cleared by an old start request.
            if (!gMotorIsNeedRun && (ErrorCode.value & persistent) == 0 &&
                (ErrorCode.value & ERROR_MASK_BLOCKING & (uint16_t)~clearMask) == 0) {
                ErrorCode.value &= (uint16_t)~clearMask;
                gMotorControlEpoch++;
                gMotorIsNeedRun = true;
            }
        } else if (gMotorIsNeedRun && mcState == mcRun &&
                   (ErrorCode.value & ERROR_MASK_BLOCKING) == 0) {
            gMotorIsNeedRun = false;
            gMotorControlEpoch++;
            gBrakeTimeoutMs = request.brakeTimeMs;
            PT_INIT(&gBrakePT);
            mcState = mcdec;
        } else {
            stopNow = true;
        }
    }
    criticalExit(irqSave);
    if (stopNow) {
        MotorEmergencyStop(0);
    }
}

void MotorStart(void) {
    publishMotorRequest(0);
}

void MotorEmergencyStop(uint16_t errorMask) {
    uint32_t irqSave = criticalEnter();
    // Do not rewrite the mailbox, avoiding contention with a preempted main loop for the slot; stale-generation requests are discarded by the ADC.
    IsrStoreRelease(&gEmergencyEpoch, gEmergencyEpoch + 1u);
    ErrorCode.value |= errorMask;
    gMotorIsNeedRun = false;
    gMotorControlEpoch++;
    HoldParm.PhaseIsChange = true;
    SYSTICK_CMP = UINT32_MAX;
    SysTick->SR &= ~(1u << 0);
    NVIC_ClearPendingIRQ(SysTicK_IRQn);
#if CMP_SUPPORT_ENABLE == 1
    DrvCmpITCmd(false);
#endif
    TIM_CtrlPWMOutputs(TIM1, DISABLE);
    PWMSetAllOff();
    MotorAdcSamplingReset(); // Release IRQ muting + rebuild heartbeat (does not revive the old plan)
    if (mcState != mcIdle) {
        mcState = mcStop;
    }
    criticalExit(irqSave);
}

// MotorStop(0) keeps a synchronous emergency stop; normal braking is handled by the next ADC control interrupt.
void MotorStop(uint32_t brakeTimeMs) {
    if (brakeTimeMs == 0) {
        MotorEmergencyStop(0);
        return;
    }

    publishMotorRequest(brakeTimeMs);
}

// MotorDrag: drive the motor (forced drag)
void MotorDrag(void) {
    uint32_t irqSave = criticalEnter();
    // Revoke the old outputs first, then perform the possibly slow parameter recalculation.
    MotorResetControl(false);
    TIM1->CH1CVR = 0;
    TIM1->CH2CVR = 0;
    TIM1->CH3CVR = 0;
#if CMP_SUPPORT_ENABLE == 1
    DrvCmpITCmd(false);
#endif
    // Revoke the closed-loop delayed commutation first, so an old compare event cannot commutate again during the drag reset.
    SYSTICK_CMP = 0xFFFFFFFF;
    SysTick->SR &= ~(1u << 0);
    NVIC_ClearPendingIRQ(SysTicK_IRQn);
    MotorAdcSamplingReset();
    criticalExit(irqSave);
#if CMP_SUPPORT_ENABLE == 1
    if (HoldParm.SensorType == CMP_SENSOR) {
        ComparatorPollingStart();
    }
#endif
    ParamReset();
    StartupReset();
    SYSTICK_CLEAR();
    HoldParm.ChangePhaseTimestamp = 0;
    uint32_t stateSave = criticalEnter();
    mcState = (gMotorIsNeedRun && (ErrorCode.value & ERROR_MASK_BLOCKING) == 0) ? mcDrag : mcStop;
    criticalExit(stateSave);
    gStartTime = TZTimeGetMillsecond();
}

// EnterRunInit: initialization for entering Run (interface kept)
void EnterRunInit(void) {
}

// MotorIsRun: whether the motor is running
bool MotorIsRun(void) {
    return gMotorIsNeedRun;
}
