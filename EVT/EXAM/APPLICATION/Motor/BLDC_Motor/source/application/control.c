/********************************** (C) COPYRIGHT  *******************************
 * File Name          : control.c
 * Author             : WCH
 * Version            : V1.0.1
 * Date               : 2026/11/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "control.h"
#include "adcsample.h"
#if CMP_SUPPORT_ENABLE == 1
#include "cmp.h"
#endif
#include "config.h"
#include "critical.h"
#include "current.h"
#include "halless.h"
#include "interface.h"
#include "isr_sync.h"
#include "motorstate.h"
#include "powerloop.h"
#include "protect.h"
#include "pwm.h"
#include "speed.h"

// With -Os a plain inline may still generate a call, so steady-state control helper functions must be expanded.
#define CONTROL_INLINE __attribute__((always_inline)) static inline

// Skip count
#define SKIP_CNT 1

static volatile int32_t gDelayChargePhaseTime = 0;

volatile uint32_t gMotorControlEpoch = 0;
static volatile int32_t gControlSeedDuty = 0;
static uint32_t gLoopAppliedEpoch = UINT32_MAX;
static uint32_t gLoopLastUs = 0;

typedef struct {
    int32_t q15;
    uint16_t count;
    int32_t point[2];
    uint32_t epoch;
    int32_t advance;
} DutyPlan_T;

typedef struct {
    DutyPlan_T plan[2];
    uint32_t index;
    volatile uint32_t state; // Ready protocol: 0 no new / 1 writing / 2 ready (ADC consumption clears it to 0)
} DutyMailbox_T;

// Sampling flags: SECOND = second point armed, DROP = conservatively drop the sample, WAIT = delay muting
#define ADC_S_SECOND  1u
#define ADC_S_DROP    2u
#define ADC_S_WAIT    4u
static DutyPlan_T gAdcPlan;        // The single currently applied snapshot, no pending plan
static volatile uint8_t gAdcFlags = 0;
static int32_t gAdcArmed = 0;      // Point currently armed in TIM3
#define ADC_HEART_POINT() ((int32_t)((PwmPeriod >> 2) > 1 ? (PwmPeriod >> 2) : 1))
CONTROL_INLINE void adcPwmApply(void);

// The startup mailbox is published only by the ADC and the run mailbox only by the main loop; immediate mode is consumed by the ADC.
static DutyMailbox_T gStartupDutyMailbox;
static DutyMailbox_T gRunDutyMailbox;
static int32_t gRequestedDuty = 0;
static uint32_t gRequestedDutyEpoch = UINT32_MAX;

// Sampling point planning scheme
// Sampling interval 30us
// The first point must avoid the leading 2us, at most two points are 30us apart
volatile int32_t gMultiSamplingPoint[2] = {0, 0};

// Adaptive phase advance: when the outer PI saturates the commutation delay factor is gradually reduced (earlier commutation), equivalent to field weakening to raise high-speed output
// Normal value _IQ15(0.6), minimum _IQ15(0.3); the smaller the value, the larger the advance angle
#define PHASE_ADVANCE_ANGLE_MAX _IQ15(0.6)
#define PHASE_ADVANCE_ANGLE_MIN _IQ15(0.3)
#define PHASE_ADVANCE_STEP _IQ15(0.0005)
#define PHASE_ADVANCE_RESTORE _IQ15(0.0001)
#ifndef CONTROL_LOOP_PERIOD_US
#define CONTROL_LOOP_PERIOD_US 1000u
#endif
// Compatible default: keep enabled when the old project's config.h does not configure the speed loop switch (original behavior)
#ifndef SPEED_LOOP_ENABLE
#define SPEED_LOOP_ENABLE 1
#endif
static volatile int32_t gPhaseAdvance = PHASE_ADVANCE_ANGLE_MAX;

// The desync criterion only uses phenomena that inertia does not allow, independent of load type:
//   1) Adjacent window period shortened by >50%: speed doubled within one electrical cycle, only possible from a false zero crossing
//   2) Adjacent 60-degree interval jump >2x within a window (including quantization margin): commutation is already disordered
//   3) Period smoothly lengthening by >50%: load deceleration (hydraulic buildup/jam), not judged directly; a following stall still reports Desync
// Quantization margin: in ADC mode zero crossings are detected per PWM period, a single interval has +/-1 period error and adjacent differences <=2 periods
#define DESYNC_JITTER_MARGIN_CNT (2u * (uint32_t)(SYSTEM_CLOCK_FREQ / PWM_FRE))
// Number of consecutive abnormal windows, filters single glitches
#ifndef DESYNC_CONFIRM_CNT
#define DESYNC_CONFIRM_CNT 2
#endif
// If a stall occurs within this time (ms) after a large smooth deceleration, report Desync and stop (same result as the original criterion)
#ifndef DESYNC_SUSPECT_TIME
#define DESYNC_SUSPECT_TIME 200
#endif
static uint32_t gDesyncLastInterval = 0; // Previous 60-degree interval, used for adjacent jump detection
static bool gDesyncJitter = false;       // An adjacent jump occurred in the current accumulation window
static bool gDesyncWindowJitter = false; // Jump conclusion of the completed window, used by run()
static uint8_t gDesyncCnt = 0;           // Count of consecutive abnormal windows
static bool gDesyncSuspect = false;      // A large smooth deceleration has occurred
static uint64_t gDesyncSuspectMs = 0;    // Time of the large smooth deceleration

// CMP/ADC automatic switch commutation-time thresholds (SysTick ticks)
// In ADC_CMP_SENSOR mode, commutation time > DOWN threshold (speed below SPEED_MIN) switches to ADC, < UP threshold switches back to CMP
// Hysteresis: UP = DOWN * 80%
#define CMP_ADC_SWITCH_TIME_DOWN ((uint32_t)((uint64_t)SYSTEM_CLOCK_FREQ * 10 / (SPEED_MIN * 3.0f)))
#define CMP_ADC_SWITCH_TIME_UP ((uint32_t)((uint64_t)CMP_ADC_SWITCH_TIME_DOWN * 8 / 10))

CONTROL_INLINE void multiSamplingCalc(int32_t q15duty, DutyPlan_T *plan);
CONTROL_INLINE uint32_t stageDutyPlan(DutyMailbox_T *mailbox, const DutyPlan_T *plan);
CONTROL_INLINE void publishDutyPlan(DutyMailbox_T *mailbox, const DutyPlan_T *plan);
CONTROL_INLINE void calcSpeedTime(void);
CONTROL_INLINE void desyncReset(void);
CONTROL_INLINE void desyncStop(void);
CONTROL_INLINE bool desyncCheck(void) __attribute__((unused));
CONTROL_INLINE void delayCharge(void);
CONTROL_INLINE int32_t updatePhaseAdvance(void);
CONTROL_INLINE void run(void);
CONTROL_INLINE bool isZeroCross(void);
CONTROL_INLINE int loop(void);
CONTROL_INLINE void cmpAdcSwitch(void);
CONTROL_INLINE void zeroCross(void);
CONTROL_INLINE void adcSample(void);

/*****************************************************************************
  Function name : MotorControl
  Description   : Motor control
  Input         : none
  Output        : void
*****************************************************************************/
FAST_FUNC __weak_symbol void MotorControl(void) {
    // DROP: the ISR has already ACKed it; only acknowledge the dropped-sample event without BEMF/state machine/PWM writes
    if ((gAdcFlags & ADC_S_DROP) != 0) {
        gAdcFlags &= (uint8_t)~(ADC_S_DROP | ADC_S_SECOND);
        return;
    }
    // Entry snapshot: oldArmed is the trigger point of this JEOC; entryCNT < oldArmed means the conversion
    // completion has crossed one PWM boundary (assumes ISR budget < one period, does not cover multi-period stalls)
    int32_t oldArmed = gAdcArmed;
    int32_t entryCNT = (int32_t)TIM1->CNT;
    MotorStateProcessRequests();
    if (mcState != mcIdle && (ErrorCode.value & ERROR_MASK_BLOCKING) != 0 &&
        (gMotorIsNeedRun || mcState == mcdec || mcState == mcReverse)) {
        MotorEmergencyStop(0);
    }
    if ((gAdcFlags & ADC_S_DROP) != 0) {
        return; // Reset triggered by a request/protection has already marked a drop, old samples do not enter control
    }
    adcSample();
    if (gMotorIsNeedRun == false && mcState != mcdec) {
        if (mcState != mcIdle) {
            mcState = mcStop;
        }
        PWMSetAllOff();
    }

    // Points are scheduled before run/state machine: sampling was already completed with the old parameters
    if ((gAdcFlags & ADC_S_WAIT) == 0) {
        int32_t now0 = (int32_t)TIM1->CNT;
        bool wrapped = (entryCNT < oldArmed) || (now0 < entryCNT);
        bool second = (gAdcFlags & ADC_S_SECOND) != 0;
        bool last = second || HoldParm.IsCmpMode == true ||
                    gAdcPlan.count == 0 || gAdcPlan.point[1] <= 0 ||
                    wrapped ||
                    (gAdcPlan.point[1] - now0 <= (int32_t)SAMPLING_TIME_CNT);
        if (last == false) {
            // A true FIRST with the second point reachable in this period: write the point now without reading a new plan
            ADC_TRIGGER_POINT(gAdcPlan.point[1]);
            gAdcArmed = gAdcPlan.point[1];
            gAdcFlags |= ADC_S_SECOND;
        } else {
            gAdcFlags &= (uint8_t)~ADC_S_SECOND;
            // Last point: consume the new plan (slow path only when READY) and then re-arm the first point
            adcPwmApply();
            int32_t first =
                (gAdcPlan.count != 0 && HoldParm.IsCmpMode == false)
                    ? gAdcPlan.point[0]
                    : ((HoldParm.IsCmpMode == true && gAdcPlan.count != 0)
                           ? (gAdcPlan.count >> 1)
                           : ADC_HEART_POINT());
            int32_t now1 = (int32_t)TIM1->CNT;
            ADC_TRIGGER_POINT(first);
            gAdcArmed = first;
            int32_t now2 = (int32_t)TIM1->CNT;
            wrapped = wrapped || (now1 < now0);
            // The sample is still in the trigger period and the new first point lies ahead of CNT: one extra
            // bridge JEOC occurs this period, so conservatively drop one; a write window crossing the boundary also allows one drop
            if ((wrapped == false && first > now1) || now2 < now1) {
                gAdcFlags |= ADC_S_DROP;
            }
        }
    }
    if (mcState == mcRun) {
        run();
    } else {
        MotorStateRun();
    }

}

CONTROL_INLINE void adcSample(void) {
    static const uint8_t highPhase[6] = {0, 0, 1, 1, 2, 2};
    static const uint8_t lowPhase[6] = {1, 2, 2, 0, 0, 1};
    static const uint8_t floatPhase[6] = {2, 1, 0, 2, 1, 0};
    int32_t phase;
    int32_t high;
    int32_t low;
    int32_t flt;

    // Current injected-group mapping
    // Channel 1: phase U
    // Channel 2: phase V
    // Channel 3: phase W
    // Channel 4: current
    ADCSample.PhaseVoltage[0] = (uint16_t)ADC1->IDATAR1;
    ADCSample.PhaseVoltage[1] = (uint16_t)ADC1->IDATAR2;
    ADCSample.PhaseVoltage[2] = (uint16_t)ADC1->IDATAR3;
    ADCSample.Current = (int32_t)ADC1->IDATAR4 - (int32_t)ADCSample.CurrentOffset;

    ADCSample.SampleErr = false;
    ADCSample.NeutralPoint = ADCSample.PhaseVoltage[0] + ADCSample.PhaseVoltage[1] + ADCSample.PhaseVoltage[2];

    // HoldParm.Phase is volatile, so take a snapshot before indexing to ensure the high/low/floating phases come from the same sector;
    // a commutation interrupt with higher preemption priority could rewrite Phase between reads, causing cross-sector mixed reads
    phase = HoldParm.Phase;
    if (phase < 0 || phase >= 6) {
        ADCSample.SampleErr = true;
        return;
    }

    high = ADCSample.PhaseVoltage[highPhase[phase]];
    low = ADCSample.PhaseVoltage[lowPhase[phase]];
    flt = ADCSample.PhaseVoltage[floatPhase[phase]];

    // Difference method: the conducting high/low phase difference gives the full-scale reference and half voltage, the floating-phase to low-phase difference gives the detection quantity;
    // high below low (switching noise/freewheeling right after commutation) is treated as a sampling error; the previous reference is kept and this sample is skipped
    if (high < low) {
        ADCSample.SampleErr = true;
        return;
    }

    ADCSample.OnPhaseHalf = (uint16_t)((high - low) >> 1);
    ADCSample.BemfRef = (uint16_t)(high - low);
    // When the floating phase is below the low phase the unsigned difference wraps to full scale; use its terminal voltage to approximate the small signal below the low side
    ADCSample.BemfPhase = (flt < low) ? (uint16_t)flt : (uint16_t)(flt - low);
}

CONTROL_INLINE void run(void) {
    if (isZeroCross() == true) {
        zeroCross();
        if ((gAdcFlags & ADC_S_WAIT) != 0) {
            return; // delay is already armed waiting for the SysTick commutation, do not continue stall/cmpSwitch
        }
    }

    // Stall detection
    if (SYSTICK_CNT > STALL_TIME_CNT) {
        // Stall right after a large smooth deceleration: jammed, report Desync and stop (keeps the original criterion result)
        // if (gDesyncSuspect == true && TZTimeGetMillsecond() - gDesyncSuspectMs <= DESYNC_SUSPECT_TIME) {
        //     desyncStop();
        //     return;
        // }
        ProtectSetError(ERROR_MASK_STALL);
        MotorDrag();
        return;
    }

    // Desync detection
    // if (HoldParm.DesyncCheck == true) {
    //     HoldParm.DesyncCheck = false;
    //     if (desyncCheck() == true) {
    //         return;
    //     }
    // }

    cmpAdcSwitch();
}

CONTROL_INLINE void zeroCross(void) {
    HoldParm.LastZeroCrossTime = HoldParm.ZeroCrossTime;
    HoldParm.ZeroCrossTime = SYSTICK_CNT;
    SYSTICK_CLEAR();
    HoldParm.ChangePhaseTimestamp = 0;
    HoldParm.PhaseIsChange = false;
    HoldParm.Count = 0;
    calcSpeedTime(); // Done before arming, so that little computation is left after interrupts are disabled
    if (HoldParm.LastZeroCrossTime == 0 || PHASE_DELAY_ENABLE == 0) {
        PWMSwitchPhase(); // Commutation
    } else {
        delayCharge();
    }
}

FAST_FUNC void MotorZeroCross(void) {
    zeroCross();
}

CONTROL_INLINE bool isZeroCross(void) {
    if (HoldParm.IsCmpMode == true) {
        return false;
    }

    if (HoldParm.PhaseIsChange == false) {
        return false;
    }

    if (ADCSample.SampleErr == true) {
        return false;
    }

    if (HoldParm.Count < SKIP_CNT) {
        HoldParm.Count++;
        return false;
    }

    if (CheckZeroCross(HoldParm.RotorDirection, HoldParm.Phase,
                       ADCSample.BemfPhase, ADCSample.BemfRef) == false) {
        return false;
    }

    // A commutation interval shorter than 30 degrees is considered an invalid signal
    // TODO enable as appropriate
    // if ((SYSTICK_CNT < ((HoldParm.LastZeroCrossTime + HoldParm.ZeroCrossTime) >> 2) + HoldParm.ChangePhaseTimestamp)) {
    //     return false;
    // }

    return true;
}

CONTROL_INLINE int32_t updatePhaseAdvance(void) {
    int32_t advance = gPhaseAdvance;
#if POWER_LOOP_ENABLE == 1
    bool saturated = PowerLoopIsSaturated();
#elif SPEED_LOOP_ENABLE == 1
    bool saturated = SpeedIsSaturated();
#elif CURRENT_LOOP_ENABLE == 1
    // Current-limit loop only: advance commutation (field weakening) is only needed when the duty cycle is already at the upper limit and the current is still below the limit.
    // Do not use CurrentIsLimiting(): increasing the advance angle when the current exceeds the limit makes the current even larger, the opposite direction
    bool saturated = CurrentIsSaturated();
#else
    bool saturated = false;
#endif
#if CURRENT_LOOP_ENABLE == 1 && (POWER_LOOP_ENABLE == 1 || SPEED_LOOP_ENABLE == 1)
    // In cascade mode the outer loop does not run while limiting, its Out is a stale value and must not be used to keep increasing the advance angle
    if (CurrentIsLimiting()) {
        saturated = false;
    }
#endif

    if (saturated) {
        advance -= PHASE_ADVANCE_STEP;
        if (advance < PHASE_ADVANCE_ANGLE_MIN) {
            advance = PHASE_ADVANCE_ANGLE_MIN;
        }
    } else {
        advance += PHASE_ADVANCE_RESTORE;
        if (advance > PHASE_ADVANCE_ANGLE_MAX) {
            advance = PHASE_ADVANCE_ANGLE_MAX;
        }
    }
    return advance;
}

CONTROL_INLINE void delayCharge(void) {
    // Set the commutation delay compensation according to the detection mode
    // CMP mode: compensate for the current CC4 input capture digital filter delay
    // ADC mode: use the fixed compensation defined by the configuration macro
    if (HoldParm.IsCmpMode == true) {
#if CMP_SUPPORT_ENABLE == 1
        HoldParm.PhaseDelayCompensation = DrvTime1Cc4GetICFilterDelay();
#endif
    } else {
        HoldParm.PhaseDelayCompensation = PHASE_DELAY_COMPENSATION_CNT;
    }

    // Signed arithmetic throughout: at high speed the compensation may exceed the delay and the result becomes negative.
    // If uint32 arithmetic were mixed in, the negative value would be written into SYSTICK_CMP as a huge unsigned number, that commutation would never happen, and after 20ms a stall would be misjudged and re-dragged
    gDelayChargePhaseTime = _IQ15mpy((int32_t)((HoldParm.ZeroCrossTime + HoldParm.LastZeroCrossTime) >> 2), gPhaseAdvance) - (int32_t)HoldParm.PhaseDelayCompensation;

    // Delayed commutation: only disable JEOCIE and let SysTick commutate; if too short, commutate in place directly
    if (MotorAdcDelayArm(gDelayChargePhaseTime) == false) {
        PWMSwitchPhase();
        return;
    }
    return;
}


/*****************************************************************************
  Function name : CalcSpeed
  Description   : Speed calculation
  Input         : none
  Output        : void
*****************************************************************************/
CONTROL_INLINE void calcSpeedTime(void) {
    // Speed feedback: refreshed on every commutation (window width is still a sliding window of 6 intervals)
    SpeedPushInterval(HoldParm.ZeroCrossTime);

    // Desync detection: an adjacent 60-degree interval jump >2x (including quantization margin) is recorded as in-window commutation disorder.
    // However heavy the load, inertia only allows smooth interval changes; alternating long and short intervals can only mean disordered commutation
    uint32_t t = HoldParm.ZeroCrossTime;
    if (gDesyncLastInterval != 0) {
        uint32_t lo = (t < gDesyncLastInterval) ? t : gDesyncLastInterval;
        uint32_t hi = (t < gDesyncLastInterval) ? gDesyncLastInterval : t;
        if (hi - lo > lo + DESYNC_JITTER_MARGIN_CNT) {
            gDesyncJitter = true;
        }
    }
    gDesyncLastInterval = t;

    // Desync detection: the non-overlapping whole-6-interval window must still be used.
    // The criterion is that two adjacent 6-interval sums differ by >50%; with a sliding window, adjacent outputs share 5/6 of the samples,
    // so the difference could never reach 50% and the desync protection would be silently disabled
    HoldParm.SpeedTime_Sum += t;
    if (++HoldParm.SpeedTime_Cnt >= 6) {
        HoldParm.ZeroCrossTimeSum = HoldParm.SpeedTime_Sum;
        HoldParm.SpeedTime_Cnt = 0;
        HoldParm.SpeedTime_Sum = 0;
        gDesyncWindowJitter = gDesyncJitter;
        gDesyncJitter = false;
        if (mcState == mcRun) {
            HoldParm.DesyncCheck = true;
        }
    }
}

// desyncReset: reset the desync criterion state (called on entry handover to mcRun)
CONTROL_INLINE void desyncReset(void) {
    gDesyncLastInterval = 0;
    gDesyncJitter = false;
    gDesyncWindowJitter = false;
    gDesyncCnt = 0;
    gDesyncSuspect = false;
}

// desyncStop: report desync and stop
CONTROL_INLINE void desyncStop(void) {
    ProtectSetError(ERROR_MASK_DESYNC);
    HoldParm.LastZeroCrossTimeSum = 0;
    HoldParm.ZeroCrossTimeSum = 0;
    desyncReset();
    MotorStop(0);
}

// The current call is commented out; the criterion is kept without changing its enabled state.
CONTROL_INLINE bool desyncCheck(void) {
    int32_t ref = HoldParm.LastZeroCrossTimeSum;
    int32_t sum = HoldParm.ZeroCrossTimeSum;

    // No reference for the first window; speeds below the value corresponding to DESYNC_TIME are not checked (same as the original criterion)
    if (ref == 0 || HoldParm.ZeroCrossTime >= DESYNC_TIME_CNT) {
        gDesyncCnt = 0;
        HoldParm.LastZeroCrossTimeSum = sum;
        return false;
    }

    // Period shortened by >50% or in-window commutation disorder: abnormal window
    if (sum < (ref >> 1) || gDesyncWindowJitter == true) {
        // Abnormal windows do not update the reference, so subsequent windows during a continuing desync are still compared with the normal window before it
        if (++gDesyncCnt >= DESYNC_CONFIRM_CNT) {
            desyncStop();
            return true;
        }
        return false;
    }

    // Period smoothly lengthening by >50%: load deceleration, not judged as desync; it is recorded so that a following stall reports Desync
    gDesyncCnt = 0;
    if (sum > ref + (ref >> 1)) {
        gDesyncSuspect = true;
        gDesyncSuspectMs = TZTimeGetMillsecond();
    }
    HoldParm.LastZeroCrossTimeSum = sum;
    return false;
}

// The main loop publishes the run duty cycle; the interrupt no longer writes this mailbox nor modifies the shared duty cycle directly.
void MotorSetDuty(int32_t q15duty) {
    DutyPlan_T plan;
    plan.epoch = IsrLoadAcquire(&gMotorControlEpoch);
    plan.advance = gPhaseAdvance;
    multiSamplingCalc(q15duty, &plan);
    gRequestedDuty = plan.q15;
    gRequestedDutyEpoch = plan.epoch;
    publishDutyPlan(&gRunDutyMailbox, &plan);
}

void MotorSetStartupDuty(int32_t q15duty) {
    DutyPlan_T plan;
    plan.epoch = IsrLoadAcquire(&gMotorControlEpoch);
    plan.advance = PHASE_ADVANCE_ANGLE_MAX;
    multiSamplingCalc(q15duty, &plan);
    publishDutyPlan(&gStartupDutyMailbox, &plan);
    // The startup algorithm and blocking pulse sampling must read back the requested value immediately; the hardware still commits at the PWM boundary.
    HoldParm.PWMDutyCycle = plan.q15;
}

CONTROL_INLINE uint32_t stageDutyPlan(DutyMailbox_T *mailbox, const DutyPlan_T *plan) {
    IsrStoreRelaxed(&mailbox->state, 1u); // Writing
    __asm volatile ("" ::: "memory");
    uint32_t index = IsrLoadRelaxed(&mailbox->index) ^ 1u;
    DutyPlan_T *pending = &mailbox->plan[index];
    pending->q15 = plan->q15;
    pending->count = plan->count;
    pending->point[0] = plan->point[0];
    pending->point[1] = plan->point[1];
    pending->epoch = plan->epoch;
    pending->advance = plan->advance;
    return index;
}

CONTROL_INLINE void publishDutyPlan(DutyMailbox_T *mailbox, const DutyPlan_T *plan) {
    uint32_t index = stageDutyPlan(mailbox, plan);
    IsrStoreRelease(&mailbox->index, index);
    IsrStoreRelease(&mailbox->state, 2u); // Mark ready after index is published
}

// Writes take effect immediately (preload disabled for both TIM1/TIM3), no pending plan
// Slow path: called only when READY. Candidate selection/epoch validation follow the original rules;
// identical plans are neither copied nor written to CCR; otherwise each field is copied to gAdcPlan and the three CCRs plus the mirrors are written immediately.
static void FAST_FUNC submitAdcPlan(void) {
    uint32_t epoch = IsrLoadAcquire(&gMotorControlEpoch);
    const DutyPlan_T *src;
    if (mcState == mcRun) {
        src = &gRunDutyMailbox.plan[IsrLoadAcquire(&gRunDutyMailbox.index)];
        if (src->epoch != epoch) {
            src = &gStartupDutyMailbox.plan[IsrLoadAcquire(&gStartupDutyMailbox.index)];
        }
    } else {
        src = &gStartupDutyMailbox.plan[IsrLoadAcquire(&gStartupDutyMailbox.index)];
    }
    const DutyPlan_T *candidate;
    DutyPlan_T zero;
    if (src->epoch == epoch) {
        candidate = src;
    } else {
        // Generation expired: use a complete safe zero plan + heartbeat, do not reuse the old non-zero plan
        zero.q15 = 0;
        zero.count = 0;
        zero.point[0] = ADC_HEART_POINT();
        zero.point[1] = 0;
        zero.epoch = epoch;
        zero.advance = PHASE_ADVANCE_ANGLE_MAX;
        candidate = &zero;
    }
    if (candidate->q15 == gAdcPlan.q15 && candidate->count == gAdcPlan.count &&
        candidate->point[0] == gAdcPlan.point[0] &&
        candidate->point[1] == gAdcPlan.point[1] &&
        candidate->epoch == gAdcPlan.epoch &&
        candidate->advance == gAdcPlan.advance) {
        // Same-value consumption: acknowledge only, do not write CCR
        IsrStoreRelaxed(&gStartupDutyMailbox.state, 0u);
        if (mcState == mcRun) {
            IsrStoreRelaxed(&gRunDutyMailbox.state, 0u);
        }
        return;
    }
    // Apply the source field by field (no whole-structure copy, no memcpy) and write the registers immediately;
    // the comparator may change the output at once - the user already chose the immediate route for waveform trade-offs
    gAdcPlan.q15 = candidate->q15;
    gAdcPlan.count = candidate->count;
    gAdcPlan.point[0] = candidate->point[0];
    gAdcPlan.point[1] = candidate->point[1];
    gAdcPlan.epoch = candidate->epoch;
    gAdcPlan.advance = candidate->advance;
    TIM1->CH1CVR = candidate->count;
    TIM1->CH2CVR = candidate->count;
    TIM1->CH3CVR = candidate->count;
    gMultiSamplingPoint[0] = candidate->point[0];
    gMultiSamplingPoint[1] = candidate->point[1];
    HoldParm.PWMDutyCycle = candidate->q15;
    gPhaseAdvance = candidate->advance;
    IsrStoreRelaxed(&gStartupDutyMailbox.state, 0u);
    if (mcState == mcRun) {
        IsrStoreRelaxed(&gRunDutyMailbox.state, 0u);
    }
}

// thin gate: safe zero plan when not driving; while driving, any state==1 keeps the current snapshot (do not read while writing);
// slow path is not called unless READY. main cannot preempt during ADC0, relaxed reads on the same core.
CONTROL_INLINE void adcPwmApply(void) {
    bool driving = gMotorIsNeedRun == true &&
                   (ErrorCode.value & ERROR_MASK_BLOCKING) == 0;
    if (driving) {
        MotorState_T st = mcState;
        driving = (st == mcAlign || st == mcDrag || st == mcRun);
    }
    if (driving == false) {
        // Not driving: zero plan + quarter-period safety heartbeat, revoke any old non-zero CCR if needed; MOE/CCER are not enabled.
        // Already normalized (zero snapshot + all CCRs off) returns directly without rewriting fields every frame;
        // all external non-zero write paths such as Reset/PulseExit/Init are cleared through Reset, so nothing is masked
        if (gAdcPlan.count == 0 && gAdcPlan.point[0] == ADC_HEART_POINT() &&
            gAdcPlan.point[1] == 0 && TIM1->CH1CVR == 0 &&
            TIM1->CH2CVR == 0 && TIM1->CH3CVR == 0) {
            return;
        }
        if (TIM1->CH1CVR != 0 || TIM1->CH2CVR != 0 || TIM1->CH3CVR != 0) {
            TIM1->CH1CVR = 0;
            TIM1->CH2CVR = 0;
            TIM1->CH3CVR = 0;
        }
        gAdcPlan.q15 = 0;
        gAdcPlan.count = 0;
        gAdcPlan.point[0] = ADC_HEART_POINT();
        gAdcPlan.point[1] = 0;
        gAdcPlan.advance = PHASE_ADVANCE_ANGLE_MAX;
        gMultiSamplingPoint[0] = gAdcPlan.point[0];
        gMultiSamplingPoint[1] = 0;
        return;
    }
    uint32_t startupState = IsrLoadRelaxed(&gStartupDutyMailbox.state);
    uint32_t runState =
        (mcState == mcRun) ? IsrLoadRelaxed(&gRunDutyMailbox.state) : 0u;
    if (startupState == 1u || runState == 1u) {
        return; // Writing: keep the current snapshot, do not read the candidate
    }
    if (startupState != 2u && runState != 2u) {
        return; // No new parameters
    }
    submitAdcPlan();
}

// Low-frequency entry: rebuild the sampling heartbeat after init/emergency stop/Init/Drag/Pulse exit.
// All call sites already have the bridge arms off; clear the 3 CCRs (immediate), set the gAdcPlan safety heartbeat, flags=FIRST|DROP,
// rewrite the TIM3 first point, clear the old JEOC + ADC pending, enable JEOCIE; trigger/DMA/MOE are untouched.
void MotorAdcSamplingReset(void) {
    uint32_t irqSave = criticalEnter();
    TIM1->CH1CVR = 0;
    TIM1->CH2CVR = 0;
    TIM1->CH3CVR = 0;
    gAdcPlan.q15 = 0;
    gAdcPlan.count = 0;
    gAdcPlan.point[0] = ADC_HEART_POINT();
    gAdcPlan.point[1] = 0;
    gAdcPlan.epoch = IsrLoadAcquire(&gMotorControlEpoch);
    gAdcPlan.advance = PHASE_ADVANCE_ANGLE_MAX;
    gMultiSamplingPoint[0] = gAdcPlan.point[0];
    gMultiSamplingPoint[1] = 0;
    gAdcFlags = ADC_S_DROP; // Conservatively drop the first result
    gAdcArmed = gAdcPlan.point[0];
    ADC_TRIGGER_POINT(gAdcPlan.point[0]);
    ADC1->STATR = ~(uint32_t)ADC_FLAG_JEOC; // Clear the flag
    NVIC_ClearPendingIRQ(ADC_IRQn);
    ADC1->CTLR1 |= ADC_JEOCIE;
    criticalExit(irqSave);
}

// Delayed-commutation muting: only when truly in the future with >1us margin, disable JEOCIE + load SYSTICK_CMP and return true;
// when too close/already passed return false without any muting so the caller commutates directly with PWMSwitchPhase (the 1us threshold follows the original decision,
// pending board verification). Only the IRQ source is touched; JEXTTRIG/ADON/regular DMA all stay unchanged.
FAST_FUNC bool MotorAdcDelayArm(uint32_t when) {
    if ((int32_t)(when - SYSTICK_CNT) <= 48) {
        return false;
    }
    gAdcFlags |= ADC_S_WAIT;
    gAdcFlags &= (uint8_t)~(ADC_S_DROP | ADC_S_SECOND);
    ADC1->CTLR1 &= (uint32_t)~ADC_JEOCIE; // Mute only the JEOC interrupt source
    SYSTICK_CMP = when;
    return true;
}

// Called after the SysTick commutation completes: resume only while in WAIT; the current plan is unchanged, the first point is rewritten,
// flags=0 (FIRST), JEOC is restored. The first valid sample after commutation is suppressed from zero-crossing detection by the existing Count/SKIP_CNT,
// so the whole IRQ is no longer dropped; in-flight old conversions finish sampling as usual.
FAST_FUNC void MotorAdcSamplingResume(void) {
    if ((gAdcFlags & ADC_S_WAIT) == 0) {
        return;
    }
    gAdcFlags = 0;
    int32_t first =
        (gAdcPlan.count != 0 && HoldParm.IsCmpMode == false)
            ? gAdcPlan.point[0]
            : ((HoldParm.IsCmpMode == true && gAdcPlan.count != 0)
                   ? (gAdcPlan.count >> 1)
                   : ADC_HEART_POINT());
    ADC_TRIGGER_POINT(first);
    gAdcArmed = first;
    ADC1->STATR = ~(uint32_t)ADC_FLAG_JEOC;
    NVIC_ClearPendingIRQ(ADC_IRQn);
    ADC1->CTLR1 |= ADC_JEOCIE;
}

// Plan the two sampling points inside the PWM on-time according to the duty cycle (from early to late)
// Sampling window: [AVOID_SWITCH, duty - SAMPLING_TIME], point spacing SAMPLING_INTERVAL
// The last point sits at the end of the on-time: the whole injected sequence completes on the turn-off edge, sampling does not cross into the freewheeling region
CONTROL_INLINE void multiSamplingCalc(int32_t q15duty, DutyPlan_T *plan) {
    if (q15duty > PWM_MAX_DUTY) {
        q15duty = PWM_MAX_DUTY;
    }
    if (q15duty < PWM_MIN_DUTY) {
        q15duty = PWM_MIN_DUTY;
    }
    int32_t duty = (int32_t)PWM_Q15_TO_CNT(q15duty);
    int32_t point1 = 0;
    int32_t point0;

    // Upper bound of usable sampling in the on-time: the injected sequence starts before the turn-off edge and ends exactly at turn-off
    int32_t sampleEnd = duty - (int32_t)SAMPLING_TIME_CNT;
    int32_t sampleStart = (int32_t)AVOID_SWITCH_CNT;

    if (sampleEnd <= sampleStart) {
        // Duty cycle too small to plan a sampling window: take a single point at the middle of the on-time
        point0 = duty >> 1;
        if (point0 < 1) {
            point0 = 1;
        }
    } else {
        // The last point is placed at the sampling upper bound (away from the switching edge), the first point one sampling interval earlier;
        // when the window cannot hold two points it degrades to a single point at the upper bound
        point1 = sampleEnd;
        point0 = sampleEnd - (int32_t)SAMPLING_INTERVAL_CNT;
        if (point0 < sampleStart) {
            point0 = point1;
            point1 = 0;
        }
    }

    plan->q15 = q15duty;
    plan->count = (uint16_t)duty;
    plan->point[0] = point0;
    plan->point[1] = point1;
}

// MotorSpeedUpdate: update the speed-related parameters
void MotorSpeedUpdate(void) {
    calcSpeedTime();
}

// Clear the speed/desync statistics so the slow commutation intervals of the startup phase are not carried into closed loop:
// when SpeedTime_Sum accumulates continuously across the handover, the first 6-commutation sum after entering mcRun
// would mix in the slow startup intervals and differ from the following acceleration values by more than 50%, causing run()'s
// desync detection to misjudge Desync and stop; the sliding window and the sensorless filter state are reset together.
void MotorResetRunFeedback(void) {
    HoldParm.SpeedTime_Cnt = 0;
    HoldParm.SpeedTime_Sum = 0;
    HoldParm.ZeroCrossTimeSum = 0;
    HoldParm.LastZeroCrossTimeSum = 0;
    HoldParm.DesyncCheck = false;
    desyncReset();
    SpeedResetWindow();
    HallessReset();
}

// Old parameter kept for interface compatibility; every mode change invalidates in-flight calculations, PI is reset by the main loop itself.
void MotorResetControl(bool resetLoop) {
    (void)resetLoop;
    DutyPlan_T plan = {0};
    plan.point[0] = PwmPeriod >> 1;
    plan.advance = PHASE_ADVANCE_ANGLE_MAX;
    uint32_t index = stageDutyPlan(&gStartupDutyMailbox, &plan);
    uint32_t irqSave = criticalEnter();
    uint32_t epoch = gMotorControlEpoch + 1u;
    gControlSeedDuty = 0;
    gStartupDutyMailbox.plan[index].epoch = epoch;
    IsrStoreRelease(&gStartupDutyMailbox.index, index);
    IsrStoreRelease(&gStartupDutyMailbox.state, 2u);
    gPhaseAdvance = PHASE_ADVANCE_ANGLE_MAX;
    IsrStoreRelease(&gMotorControlEpoch, epoch);
    criticalExit(irqSave);
}

void MotorEnterRun(void) {
    DutyPlan_T plan;
    multiSamplingCalc(HoldParm.PWMDutyCycle, &plan);
    plan.epoch = 0;
    plan.advance = gPhaseAdvance;
    uint32_t index = stageDutyPlan(&gStartupDutyMailbox, &plan);
    uint32_t irqSave = criticalEnter();
    if (gMotorIsNeedRun && (ErrorCode.value & ERROR_MASK_BLOCKING) == 0) {
        uint32_t epoch = gMotorControlEpoch + 1u;
        gControlSeedDuty = plan.q15;
        // Before the first main-loop plan arrives, keep using the startup duty cycle of the same generation.
        gStartupDutyMailbox.plan[index].epoch = epoch;
        IsrStoreRelease(&gStartupDutyMailbox.index, index);
        IsrStoreRelease(&gMotorControlEpoch, epoch);
        IsrStoreRelease(&gStartupDutyMailbox.state, 2u);
        mcState = mcRun;
        ErrorCode.value &= (uint16_t)~ERROR_MASK_STALL;
    }
    else {
        IsrStoreRelaxed(&gStartupDutyMailbox.state, 0u); // Recover when the condition fails
    }
    criticalExit(irqSave);
}

// cmpAdcSwitch automatically switches CMP/ADC according to the commutation time in ADC_CMP_SENSOR mode
// Commutation time > CMP_ADC_SWITCH_TIME_DOWN (speed < SPEED_MIN) switches to ADC
// Commutation time < CMP_ADC_SWITCH_TIME_UP   (speed > 1.25*SPEED_MIN) switches back to CMP
// Hysteresis prevents frequent switching
CONTROL_INLINE void cmpAdcSwitch(void) {
#if CMP_SUPPORT_ENABLE == 1
    if (HoldParm.SensorType != ADC_CMP_SENSOR) {
        return;
    }

    if (HoldParm.ZeroCrossTime == 0) {
        return;
    }

    // The whole switch must be atomic: this function runs in the ADC interrupt (preemption priority 1), while TIM1_CC4 (priority 0)
    // can preempt it and call DrvCmpITCmd(false). If the preemption lands between IsCmpMode and the CMP register configuration,
    // an inconsistent state appears where "software thinks it is in CMP mode but the hardware has not enabled the CC4 interrupt"
    if (HoldParm.IsCmpMode == true) {
        if (HoldParm.ZeroCrossTime > CMP_ADC_SWITCH_TIME_DOWN) {
            uint32_t irqSave = criticalEnter();
            DrvCmpITCmd(false);
            HoldParm.IsCmpMode = false;
            criticalExit(irqSave);
            HallessReset();
        }
    } else {
        if (HoldParm.ZeroCrossTime < CMP_ADC_SWITCH_TIME_UP) {
            uint32_t irqSave = criticalEnter();
            HoldParm.IsCmpMode = true;
            CmpChannelCharge();
            DrvCmpITCmd(true);
            criticalExit(irqSave);
        }
    }
#endif
}

// MotorWhile: motor main loop
void MotorWhile(void) {
    if (gMotorIsNeedRun && mcState == mcRun) {
        loop();
    }
}

// MotorLoop: motor loop
void MotorLoop(void) {
    if (mcState == mcRun) {
        loop();
    }
}

CONTROL_INLINE int loop(void) {
    uint32_t epoch = IsrLoadAcquire(&gMotorControlEpoch);
    int32_t seed = gControlSeedDuty;
    bool valid = gMotorIsNeedRun && mcState == mcRun &&
                 (ErrorCode.value & ERROR_MASK_BLOCKING) == 0;
    IsrAcquireFence();
    if (!valid || epoch != IsrLoadRelaxed(&gMotorControlEpoch)) {
        return 0;
    }

    uint32_t now = (uint32_t)TZTimeGet();
    if (gLoopAppliedEpoch != epoch) {
        // The only writer of PI is the main loop; the interrupt only publishes the generation and initial values.
        SpeedResetControl();
        CurrentReset();
        PowerLoopReset();
        SpeedSetUI(seed);
        CurrentSetUI(seed);
        PowerLoopSetUI(seed);
        gLoopAppliedEpoch = epoch;
        gLoopLastUs = now - CONTROL_LOOP_PERIOD_US;
    }
    SpeedCalc();
    if (now - gLoopLastUs < CONTROL_LOOP_PERIOD_US) {
        return 0;
    }
    gLoopLastUs += CONTROL_LOOP_PERIOD_US;
    if (now - gLoopLastUs >= CONTROL_LOOP_PERIOD_US) {
        gLoopLastUs = now;
    }
    int32_t out = 0;

#if CURRENT_LOOP_ENABLE == 1
    // Inner loop: the current limit loop always runs
    CurrentRun();

    if (CurrentIsLimiting()) {
        // Current over limit: use the inner loop output and feed it to the outer loop integrator to prevent integral windup
        out = CurrentGetOutput();
#if POWER_LOOP_ENABLE == 1
        PowerLoopSetUI(out);
#elif SPEED_LOOP_ENABLE == 1
        SpeedSetUI(out);
#endif
    } else
#endif
    {
        // Current normal: run the outer loop (power loop > speed loop > current-limit-only constant current > open loop)
#if POWER_LOOP_ENABLE == 1
        PowerLoopRun();
        out = PowerLoopGetOutput();
#elif SPEED_LOOP_ENABLE == 1
        SpeedRun();
        out = SpeedGetOutput();
#elif CURRENT_LOOP_ENABLE == 1
        // Current limit loop only: constant current control, the PI output is used directly as the duty cycle
        out = CurrentGetOutput();
#else
        // All outer loops disabled: run with an open-loop duty cycle, the application adjusts speed via MotorSetDuty
        out = gRequestedDutyEpoch == epoch ? gRequestedDuty : seed;
#endif
#if CURRENT_LOOP_ENABLE == 1 && (POWER_LOOP_ENABLE == 1 || SPEED_LOOP_ENABLE == 1)
        // Cascade mode: the outer loop output is fed back to the current limit loop integrator to prevent windup; with only the current limit loop the integrator accumulates naturally without feedback
        CurrentSetUI(out);
#endif
    }

    int32_t advance = updatePhaseAdvance();
    DutyPlan_T plan;
    multiSamplingCalc(out, &plan);
    plan.epoch = epoch;
    plan.advance = advance;
    publishDutyPlan(&gRunDutyMailbox, &plan);
    return 0;
}

// MotorGetDuty: get the current duty cycle
int32_t MotorGetDuty(void) {
    return HoldParm.PWMDutyCycle;
}
