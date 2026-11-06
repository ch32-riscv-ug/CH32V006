/********************************** (C) COPYRIGHT  *******************************
 * File Name          : pwm.c
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
#include "adcsample.h"
#if CMP_SUPPORT_ENABLE == 1
#include "cmp.h"
#endif

volatile uint16_t PwmPeriod = PWM_FRE_SETATA;
volatile uint32_t PwmPeriodUs = 1000000uL / PWM_FRE;

typedef union {
    struct {
        uint16_t CC1E : 1;
        uint16_t CC1P : 1;
        uint16_t CC1NE : 1;
        uint16_t CC1NP : 1;
        uint16_t CC2E : 1;
        uint16_t CC2P : 1;
        uint16_t CC2NE : 1;
        uint16_t CC2NP : 1;
        uint16_t CC3E : 1;
        uint16_t CC3P : 1;
        uint16_t CC3NE : 1;
        uint16_t CC3NP : 1;
        uint16_t CC4E : 1;
        uint16_t CC4P : 1;
        uint16_t reserved : 2;
    } bit;

    uint16_t value;
} time1Ccer;

typedef union {
    struct
    {
        uint16_t reserved1 : 4;
        uint16_t OC1M : 3;
        uint16_t reserved2 : 5;
        uint16_t OC2M : 3;
        uint16_t OC2CE : 1;
    } bit;

    uint16_t value;
} time1chctlr1;

typedef union {
    struct {
        uint16_t reserved1 : 4;
        uint16_t OC3M : 3;
        uint16_t reserved2 : 5;
        uint16_t OC4M : 3;
        uint16_t OC4CE : 1;
    } bit;

    uint16_t value;
} time1chctlr2;

// CCER: CC1E=bit0, CC1NE=bit2, CC2E=bit4, CC2NE=bit6, CC3E=bit8, CC3NE=bit10
// CHCTLR1: OC1M=bit[6:4], OC2M=bit[14:12]
// CHCTLR2: OC3M=bit[6:4]
#define CCER_CLEAR_MASK 0x0555u
#define CHCTLR1_CLEAR_MASK 0x7070u
#define CHCTLR2_CLEAR_MASK 0x0070u

// CCER setting values (hphase, lphase)
// Normal mode: only the low-side phase CC?NE is enabled, the high-side phase CC?NE stays off
// hphase=1,lphase=2: CC1E+CC2NE
#define CCER_H1L2_NORM 0x0041u
// hphase=1,lphase=3: CC1E+CC3NE
#define CCER_H1L3_NORM 0x0401u
// hphase=2,lphase=3: CC2E+CC3NE
#define CCER_H2L3_NORM 0x0410u
// hphase=2,lphase=1: CC2E+CC1NE
#define CCER_H2L1_NORM 0x0014u
// hphase=3,lphase=1: CC3E+CC1NE
#define CCER_H3L1_NORM 0x0104u
// hphase=3,lphase=2: CC3E+CC2NE
#define CCER_H3L2_NORM 0x0140u

// Complementary mode: additionally enable CC?NE of the high-side phase on top of normal mode
// hphase=1,lphase=2: CC1E+CC1NE+CC2NE
#define CCER_H1L2_COMP 0x0045u
// hphase=1,lphase=3: CC1E+CC1NE+CC3NE
#define CCER_H1L3_COMP 0x0405u
// hphase=2,lphase=3: CC2E+CC2NE+CC3NE
#define CCER_H2L3_COMP 0x0450u
// hphase=2,lphase=1: CC2E+CC2NE+CC1NE
#define CCER_H2L1_COMP 0x0054u
// hphase=3,lphase=1: CC3E+CC3NE+CC1NE
#define CCER_H3L1_COMP 0x0504u
// hphase=3,lphase=2: CC3E+CC3NE+CC2NE
#define CCER_H3L2_COMP 0x0540u

#define CCER_OFF 0x0000u

// CHCTLR1: OC1M=0x06,OC2M=0x05 / OC1M=0x06,OC2M=0x06 / OC1M=0x05,OC2M=0x06
#define CHCTLR1_L2 0x5060u // OC1M=0x06, OC2M=0x05
#define CHCTLR1_L1 0x6050u // OC1M=0x05, OC2M=0x06
#define CHCTLR1_XX 0x6060u // OC1M=0x06, OC2M=0x06

// CHCTLR2: OC3M=0x05 / OC3M=0x06
#define CHCTLR2_L3 0x0050u // OC3M=0x05
#define CHCTLR2_XX 0x0060u // OC3M=0x06

// Commutation register table entry
typedef struct {
    uint16_t ccer_norm; // CCER value in normal mode
    uint16_t ccer_comp; // CCER value in complementary mode
    uint16_t chctlr1;   // CHCTLR1 value
    uint16_t chctlr2;   // CHCTLR2 value
} PhaseRegSet_T;

// Commutation table: phaseTable[Phase]
// Phase: 0~5, CW increments, CCW decrements
// AB-AC-BC-BA-CA-CB
static const PhaseRegSet_T phaseTable[6] = {
    {CCER_H1L2_NORM, CCER_H1L2_COMP, CHCTLR1_L2, CHCTLR2_XX}, // 0: U+ V- (h1,l2)
    {CCER_H1L3_NORM, CCER_H1L3_COMP, CHCTLR1_XX, CHCTLR2_L3}, // 1: U+ W- (h1,l3)
    {CCER_H2L3_NORM, CCER_H2L3_COMP, CHCTLR1_XX, CHCTLR2_L3}, // 2: V+ W- (h2,l3)
    {CCER_H2L1_NORM, CCER_H2L1_COMP, CHCTLR1_L1, CHCTLR2_XX}, // 3: V+ U- (h2,l1)
    {CCER_H3L1_NORM, CCER_H3L1_COMP, CHCTLR1_L1, CHCTLR2_XX}, // 4: W+ U- (h3,l1)
    {CCER_H3L2_NORM, CCER_H3L2_COMP, CHCTLR1_L2, CHCTLR2_XX}, // 5: W+ V- (h3,l2)
};

static inline void time1Set(uint8_t hphase, uint8_t lphase);

// Commutation critical section note:
// HoldParm is volatile, so every reference to HoldParm.Phase in applyPhaseRegs() is an independent memory read,
// not a single value cached by the compiler. When PWMSwitchPhase() runs in the ADC interrupt (preemption priority 1),
// it may be interrupted by SysTick/TIM1_UP/TIM1_BRK/TIM1_CC4 (preemption priority 0); if the interruption lands between
// the ++/--HoldParm.Phase increment and the wraparound bounds check, and the preempting code also calls PWMSwitchPhase(),
// it may read Phase==6 (CW) or Phase==-1 (CCW) and use that out-of-range index to access phaseTable[6] (the array has only
// 6 elements), writing out-of-range memory into TIM1 as commutation register values. Even if the wraparound check itself is
// not interrupted, if the interruption lands between the three statements that compute ccer_set/chctlr1_set/chctlr2_set in
// applyPhaseRegs(), the interrupted call mixes the "Phase before the interrupt" with the "Phase modified by the nested call",
// so that CHCTLR1/CHCTLR2/CCER finally receive mutually inconsistent commutation combinations.
//
// The callers (PWMSwitchPhase/PWMApplyPhase) are responsible for wrapping the call with criticalEnter/criticalExit;
// this function does not lock again
FAST_FUNC static inline void applyPhaseRegs(void) {
    int32_t phase = HoldParm.Phase; // Take one snapshot inside the critical section, all later uses read this local value

    uint16_t ccer_set = HoldParm.IsSyncChargingMode ? phaseTable[phase].ccer_comp : phaseTable[phase].ccer_norm;
    uint16_t chctlr1_set = phaseTable[phase].chctlr1;
    uint16_t chctlr2_set = phaseTable[phase].chctlr2;

    TIM1->CHCTLR1 = (TIM1->CHCTLR1 & ~CHCTLR1_CLEAR_MASK) | chctlr1_set;
    TIM1->CHCTLR2 = (TIM1->CHCTLR2 & ~CHCTLR2_CLEAR_MASK) | chctlr2_set;
    TIM1->CCER = (TIM1->CCER & ~CCER_CLEAR_MASK) | ccer_set;

    HoldParm.PhaseIsChange = true;
    HoldParm.ChangePhaseTimestamp = SysTick->CNT;
#if CMP_SUPPORT_ENABLE == 1
    if (HoldParm.IsCmpMode == true) {
        CmpChannelCharge();
        // Commutation must continue during mcdec (brake deceleration), otherwise SysTick stops being cleared and the stall protection is triggered falsely
        if (mcState == mcRun || mcState == mcdec) {
            DrvCmpITCmd(true);
        }
    }
#endif
}

/*****************************************************************************
  Function name : PWMSwitchPhase
  Description   : PWM commutation function [CW:AB-AC-BC-BA-CA-CB	CCW:AB-CB-CA-BA-BC-AC]
  Input         : none
  Output        : void
*****************************************************************************/
FAST_FUNC void PWMSwitchPhase(void) {
    uint32_t irqSave = criticalEnter();

    if (HoldParm.RotorDirection == CW) {
        if (++HoldParm.Phase > 5) {
            HoldParm.Phase = 0;
        }
    } else {
        if (--HoldParm.Phase < 0) {
            HoldParm.Phase = 5;
        }
    }

    applyPhaseRegs();

    criticalExit(irqSave);
}

// PWMApplyPhase outputs the registers directly for the current HoldParm.Phase without switching the phase
// Used for the first power-on output when an initial phase has been preset externally (e.g. after static pulse injection detection)
FAST_FUNC void PWMApplyPhase(void) {
    uint32_t irqSave = criticalEnter();
    applyPhaseRegs();
    criticalExit(irqSave);
}

FAST_FUNC void PWMSetPhase(uint8_t hphase, uint8_t lphase) {
    time1Set(hphase, lphase);
}

// Note: the tmpccer / tmpchctlr variables in time1Set / PWMSetLonHoff / PWMSetAllOff / PWMCharge below
// used to be static. These functions are called both by MotorControl() in the ADC interrupt and by ParamLoad()
// in the main loop, so static made both contexts share the same scratch copy - when the interrupt landed between
// "read registers" and "write registers back", the interrupted side would write the other side's half-computed
// values into TIM1. As automatic variables they are naturally reentrant and save RAM as well.

// 1:U 2:V 3:W
// hphase: high-side conducting phase lphase: low-side conducting phase
static inline void time1Set(uint8_t hphase, uint8_t lphase) {
    time1Ccer tmpccer;
    tmpccer.value = TIM1->CCER;

    switch (hphase) {
    case 1:
        tmpccer.bit.CC1E = 1;
        tmpccer.bit.CC2E = 0;
        tmpccer.bit.CC3E = 0;
        break;
    case 2:
        tmpccer.bit.CC1E = 0;
        tmpccer.bit.CC2E = 1;
        tmpccer.bit.CC3E = 0;
        break;
    case 3:
        tmpccer.bit.CC1E = 0;
        tmpccer.bit.CC2E = 0;
        tmpccer.bit.CC3E = 1;
        break;
    default:
        tmpccer.bit.CC1E = 0;
        tmpccer.bit.CC2E = 0;
        tmpccer.bit.CC3E = 0;
        break;
    }

    // if (HoldParm.IsSyncChargingMode) {
    //     tmpccer.bit.CC1NE = tmpccer.bit.CC1E;
    //     tmpccer.bit.CC2NE = tmpccer.bit.CC2E;
    //     tmpccer.bit.CC3NE = tmpccer.bit.CC3E;
    // } else {
        tmpccer.bit.CC1NE = 0;
        tmpccer.bit.CC2NE = 0;
        tmpccer.bit.CC3NE = 0;
    // }

    time1chctlr1 tmpchctlr1;
    tmpchctlr1.value = TIM1->CHCTLR1;
    time1chctlr2 tmpchctlr2;
    tmpchctlr2.value = TIM1->CHCTLR2;

    switch (lphase) {
    case 1:
        tmpchctlr1.bit.OC1M = 0x05;
        tmpccer.bit.CC1NE = 1;
        tmpchctlr1.bit.OC2M = 0x06;
        tmpchctlr2.bit.OC3M = 0x06;
        break;
    case 2:
        tmpchctlr1.bit.OC1M = 0x06;
        tmpchctlr1.bit.OC2M = 0x05;
        tmpccer.bit.CC2NE = 1;
        tmpchctlr2.bit.OC3M = 0x06;
        break;
    case 3:
        tmpchctlr1.bit.OC1M = 0x06;
        tmpchctlr1.bit.OC2M = 0x06;
        tmpchctlr2.bit.OC3M = 0x05;
        tmpccer.bit.CC3NE = 1;
        break;
    default:
        tmpchctlr1.bit.OC1M = 0x06;
        tmpchctlr1.bit.OC2M = 0x06;
        tmpchctlr2.bit.OC3M = 0x06;
        break;
    }

    TIM1->CHCTLR1 = tmpchctlr1.value;
    TIM1->CHCTLR2 = tmpchctlr2.value;
    TIM1->CCER = tmpccer.value;
}

/*****************************************************************************
  Function name : PWMChangeDuty
  Description   : PWM duty cycle update
  Input         : u16data
  Output        : void
*****************************************************************************/
FAST_FUNC void PWMChangeDuty(void) {
    uint16_t cnt = PWM_Q15_TO_CNT(HoldParm.PWMDutyCycle);

    if (cnt == TIM1->CH1CVR) {
        return;
    }

    if (cnt > PwmPeriod) {
        cnt = PwmPeriod;
    }

    TIM1->CH1CVR = cnt;
    TIM1->CH2CVR = cnt;
    TIM1->CH3CVR = cnt;
}

// PWMSetFrequency: change the PWM frequency at run time
void PWMSetFrequency(uint32_t freqHz) {
    PwmPeriod = (uint16_t)(SYSTEM_CLOCK_FREQ / freqHz - 1);
    TIM1->ATRLR = PwmPeriod;
    PwmPeriodUs = 1000000uL / freqHz;
}

// PWMSetLonHoff: enable low-level chopping
void PWMSetLonHoff(void) {
    time1chctlr1 tmpchctlr1;
    tmpchctlr1.value = TIM1->CHCTLR1;
    time1chctlr2 tmpchctlr2;
    tmpchctlr2.value = TIM1->CHCTLR2;

    time1Ccer tmpccer;
    tmpccer.value = TIM1->CCER;
    tmpccer.bit.CC1E = 0;
    tmpccer.bit.CC2E = 0;
    tmpccer.bit.CC3E = 0;
    tmpccer.bit.CC1NE = 1;
    tmpccer.bit.CC2NE = 1;
    tmpccer.bit.CC3NE = 1;
    tmpchctlr1.bit.OC1M = 0x05;
    tmpchctlr1.bit.OC2M = 0x05;
    tmpchctlr2.bit.OC3M = 0x05;
    TIM1->CCER = tmpccer.value;
    TIM1->CHCTLR1 = tmpchctlr1.value;
    TIM1->CHCTLR2 = tmpchctlr2.value;
}

// PWMSetAllOff: turn all outputs off
void PWMSetAllOff(void) {
    time1Ccer tmpccer;

    TIM1->CH1CVR = 0;
    TIM1->CH2CVR = 0;
    TIM1->CH3CVR = 0;

    time1chctlr1 tmpchctlr1;
    tmpchctlr1.value = TIM1->CHCTLR1;
    time1chctlr2 tmpchctlr2;
    tmpchctlr2.value = TIM1->CHCTLR2;

    tmpchctlr1.bit.OC1M = 0x06;
    tmpchctlr1.bit.OC2M = 0x06;
    tmpchctlr2.bit.OC3M = 0x06;

    tmpccer.value = TIM1->CCER;
    tmpccer.bit.CC1E = 0;
    tmpccer.bit.CC2E = 0;
    tmpccer.bit.CC3E = 0;
    tmpccer.bit.CC1NE = 0;
    tmpccer.bit.CC2NE = 0;
    tmpccer.bit.CC3NE = 0;
    TIM1->CCER = tmpccer.value;
    TIM1->CHCTLR1 = tmpchctlr1.value;
    TIM1->CHCTLR2 = tmpchctlr2.value;
}

// PWMCharge: low-side precharge
void PWMCharge(uint8_t phase) {
    time1Ccer tmpccer;
    tmpccer.value = TIM1->CCER;
    tmpccer.bit.CC1E = 0;
    tmpccer.bit.CC2E = 0;
    tmpccer.bit.CC3E = 0;
    tmpccer.bit.CC1NE = 0;
    tmpccer.bit.CC2NE = 0;
    tmpccer.bit.CC3NE = 0;

    time1chctlr1 tmpchctlr1;
    tmpchctlr1.value = TIM1->CHCTLR1;
    time1chctlr2 tmpchctlr2;
    tmpchctlr2.value = TIM1->CHCTLR2;

    switch (phase) {
    case 1:
        tmpccer.bit.CC1NE = 1;
        tmpchctlr1.bit.OC1M = 0x05;
        tmpchctlr1.bit.OC2M = 0x06;
        tmpchctlr2.bit.OC3M = 0x06;
        break;
    case 2:
        tmpccer.bit.CC2NE = 1;
        tmpchctlr1.bit.OC1M = 0x06;
        tmpchctlr1.bit.OC2M = 0x05;
        tmpchctlr2.bit.OC3M = 0x06;
        break;
    case 3:
        tmpccer.bit.CC3NE = 1;
        tmpchctlr1.bit.OC1M = 0x06;
        tmpchctlr1.bit.OC2M = 0x06;
        tmpchctlr2.bit.OC3M = 0x05;
        break;
    default:
        break;
    }
    TIM1->CCER = tmpccer.value;
    TIM1->CHCTLR1 = tmpchctlr1.value;
    TIM1->CHCTLR2 = tmpchctlr2.value;
}


/*****************************************************************************
  Function name : PWMTest
  Description   : Output three 50% complementary high/low PWM channels to verify gate drive/dead-time waveforms
                  Disables the ADC/SysTick interrupts that would modify PWM registers, so MotorControl cannot override them
  Input         : none
  Output        : void
*****************************************************************************/
FAST_FUNC void PWMTest(void) {
    uint16_t cnt = PWM_Q15_TO_CNT(_IQ15(0.5));

    if (cnt > PwmPeriod) {
        cnt = PwmPeriod;
    }

    if (cnt == 0) {
        cnt = 1; // Prevent the complementary outputs from not toggling when the duty cycle is 0
    }

    // Prevent the motor control functions in the ADC/SysTick interrupts from overwriting the PWM outputs
    NVIC_DisableIRQ(ADC_IRQn);
    NVIC_DisableIRQ(SysTicK_IRQn);

    // Ensure TIM1 is running and the main outputs are enabled
    TIM_Cmd(TIM1, ENABLE);
    TIM_CtrlPWMOutputs(TIM1, ENABLE);

    // All three high-side CCRs at 50%
    TIM1->CH1CVR = cnt;
    TIM1->CH2CVR = cnt;
    TIM1->CH3CVR = cnt;

    // Enable all three high/low main outputs and complementary outputs
    time1Ccer tmpccer;
    tmpccer.value = TIM1->CCER;
    tmpccer.bit.CC1E = 1;
    tmpccer.bit.CC2E = 1;
    tmpccer.bit.CC3E = 1;
    tmpccer.bit.CC1NE = 1;
    tmpccer.bit.CC2NE = 1;
    tmpccer.bit.CC3NE = 1;
    TIM1->CCER = tmpccer.value;

    // All three high-side channels in PWM1 mode, the low side follows the complementary output
    time1chctlr1 tmpchctlr1;
    tmpchctlr1.value = TIM1->CHCTLR1;
    tmpchctlr1.bit.OC1M = 0x06;
    tmpchctlr1.bit.OC2M = 0x06;
    TIM1->CHCTLR1 = tmpchctlr1.value;

    time1chctlr2 tmpchctlr2;
    tmpchctlr2.value = TIM1->CHCTLR2;
    tmpchctlr2.bit.OC3M = 0x06;
    TIM1->CHCTLR2 = tmpchctlr2.value;
}
