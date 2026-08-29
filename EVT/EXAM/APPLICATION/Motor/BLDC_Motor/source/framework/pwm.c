/********************************** (C) COPYRIGHT  *******************************
 * File Name          : pwm.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2026/08/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "pwm.h"
#include "launch.h"
#include "common.h"

static volatile uint16_t period = 0;

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

static inline void time1Set(uint8_t hphase, uint8_t lphase);

/*****************************************************************************
 函 数 名  : PWMSwitchPhase
 功能描述  : PWM0换相函数[CW:AB-AC-BC-BA-CA-CB	CCW:AB-CB-CA-BA-BC-AC]
 输入参数  : 无
 输出参数  : void
*****************************************************************************/
FAST_FUNC void PWMSwitchPhase(void) {
    if (HoldParm.RotorDirection == CW) {
        switch (HoldParm.Phase) {
        case 0: // U+ V-
            time1Set(1, 2);
            break;
        case 1: // U+ W-
            time1Set(1, 3);
            break;
        case 2: // V+ W-
            time1Set(2, 3);
            break;
        case 3: // V+ U-
            time1Set(2, 1);
            break;
        case 4: // W+ U-
            time1Set(3, 1);
            break;
        case 5: // W+ V-
            time1Set(3, 2);
            break;
        default:
            time1Set(0, 0);
            break;
        }
    } else if (HoldParm.RotorDirection == CCW) {
        switch (HoldParm.Phase) {
        case 0: // U+ V-
            time1Set(1, 2);
            break;
        case 1: // W+ V-
            time1Set(3, 2);
            break;
        case 2: // W+ U-
            time1Set(3, 1);
            break;
        case 3: // V+ U-
            time1Set(2, 1);
            break;
        case 4: // V+ W-
            time1Set(2, 3);
            break;
        case 5: // U+ W-
            time1Set(1, 3);
            break;
        default:
            time1Set(0, 0);
            break;
        }
    }
    HoldParm.PhaseIsChange = true;
    HoldParm.ChangePhaseTimestamp = SysTick->CNT;

#if CMP_SUPPORT_ENABLE == 1
    if (HoldParm.IsCmpMode == true) {
        CmpChannelCharge();
    }
#endif
}

// 1:U 2:V 3:W
// hphase: 上管导通相 lphase: 下管导通相
static inline void time1Set(uint8_t hphase, uint8_t lphase) {
    static time1Ccer tmpccer;
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

#if SYNC_CHARGING_SWITCH == 1
    tmpccer.bit.CC1NE = tmpccer.bit.CC1E;
    tmpccer.bit.CC2NE = tmpccer.bit.CC2E;
    tmpccer.bit.CC3NE = tmpccer.bit.CC3E;
#else
    tmpccer.bit.CC1NE = 0;
    tmpccer.bit.CC2NE = 0;
    tmpccer.bit.CC3NE = 0;
#endif

    static time1chctlr1 tmpchctlr1;
    tmpchctlr1.value = TIM1->CHCTLR1;
    static time1chctlr2 tmpchctlr2;
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
 函 数 名  : PWMChangeDuty
 功能描述  : PWM0占空比更新
 输入参数  : u16data
 输出参数  : void
*****************************************************************************/
FAST_FUNC void PWMChangeDuty(uint16_t u16data) {
    if (u16data == period && u16data == TIM1->CH1CVR) {
        return;
    }

    if (u16data < PWM_MIN_DUTY) {
        u16data = PWM_MIN_DUTY;
    }
    period = u16data;

    TIM1->CH1CVR = period;
    TIM1->CH2CVR = period;
    TIM1->CH3CVR = period;
}

// PWMSetLonHoff 设置低电平斩波
void PWMSetLonHoff(void) {
    static time1chctlr1 tmpchctlr1;
    tmpchctlr1.value = TIM1->CHCTLR1;
    static time1chctlr2 tmpchctlr2;
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

// PWMSetAllOff 设置低电平斩波
void PWMSetAllOff(void) {
    time1Ccer tmpccer;

    TIM1->CH1CVR = 0;
    TIM1->CH2CVR = 0;
    TIM1->CH3CVR = 0;

    static time1chctlr1 tmpchctlr1;
    tmpchctlr1.value = TIM1->CHCTLR1;
    static time1chctlr2 tmpchctlr2;
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

// PWMCharge 下管预充电
void PWMCharge(uint8_t phase) {
    time1Ccer tmpccer;
    tmpccer.value = TIM1->CCER;
    tmpccer.bit.CC1E = 0;
    tmpccer.bit.CC2E = 0;
    tmpccer.bit.CC3E = 0;
    tmpccer.bit.CC1NE = 0;
    tmpccer.bit.CC2NE = 0;
    tmpccer.bit.CC3NE = 0;

    static time1chctlr1 tmpchctlr1;
    tmpchctlr1.value = TIM1->CHCTLR1;
    static time1chctlr2 tmpchctlr2;
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
