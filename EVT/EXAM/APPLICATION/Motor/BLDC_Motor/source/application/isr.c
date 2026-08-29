/********************************** (C) COPYRIGHT  *******************************
 * File Name          : isr.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2026/08/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "isr.h"
#include "common.h"
#include "control.h"
#include "pwm.h"
#include "adcsample.h"


volatile uint64_t gSysTimeUs = 0;

void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM1_UP_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM1_BRK_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

/*****************************************************************************
 函 数 名  : ADC_ISR
 功能描述  : ADC中断
 输入参数  : 无
 输出参数  : void
*****************************************************************************/
#ifdef ADC2
void ADC1_2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
FAST_FUNC void ADC1_2_IRQHandler(void) {
#else
void ADC1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
FAST_FUNC void ADC1_IRQHandler(void) {
#endif
    if (ADC_GetITStatus(ADC1, ADC_IT_JEOC) != RESET) {
        MotorControl();
        ADC_ClearFlag(ADC1, ADC_FLAG_JEOC);
    }
}

FAST_FUNC void TIM1_UP_IRQHandler(void) {
    if (TIM_GetITStatus(TIM1, TIM_IT_Update) != RESET) {
        if (HoldParm.IsCmpMode == false) {
            if (gMultiSamplingPoint[0] < PWM_MIN_DUTY) {
                AdcTriggerPoint(PWM_MIN_DUTY - SAMPLING_TIME_CNT);
            } else {
                AdcTriggerPoint(gMultiSamplingPoint[0]);
            }
            gMultiSamplingPointNow = 0;
        }

        TIM_ClearFlag(TIM1, TIM_FLAG_Update);
    }

    gSysTimeUs += (uint64_t)(1e6 / PWM_FRE);
}

FAST_FUNC void TIM1_BRK_IRQHandler(void) {
    if (TIM_GetITStatus(TIM1, TIM_IT_Break) != RESET) {
        gMotorIsNeedRun = false;
        ErrorCode.bit.OverCurrentCMP = 1;
        PWMSetAllOff();
        TIM_CtrlPWMOutputs(TIM1, ENABLE);
        TIM_ClearFlag(TIM1, TIM_FLAG_Break);
    }
}

FAST_FUNC void SysTick_Handler(void) {
    if (HoldParm.PhaseIsChange == false) {
        if (gMotorIsNeedRun == true) {
            PWMSwitchPhase(); // 换相
        }
        SysTick->CMP = STALL_TIME * (SYSTEM_CLOCK_FREQ / 1000);
    } else if (SysTick->CNT > STALL_TIME * (SYSTEM_CLOCK_FREQ / 1000)) {
        if (mcState == mcRun && gMotorIsNeedRun == true) {
            mcState = mcDrag;
        }
    }

    SysTick->SR &= ~(1 << 0);
}

// SysTimeGet 获取系统时间
FAST_FUNC uint64_t SysTimeGet(void) {
    static volatile uint64_t t1, t2;
    for (;;) {
        t1 = gSysTimeUs;
        t2 = gSysTimeUs;
        if (t1 == t2) {
            break;
        }
        if (t1 < t2 && t2 - t1 <= 100) {
            break;
        }
    }

    return t2;
}

/*********************************************************************
 * @fn      NMI_Handler
 *
 * @brief   This function handles NMI exception.
 *
 * @return  none
 */
void NMI_Handler(void) {
    while (1) {
    }
}

/*********************************************************************
 * @fn      HardFault_Handler
 *
 * @brief   This function handles Hard Fault exception.
 *
 * @return  none
 */
void HardFault_Handler(void) {
    NVIC_SystemReset();
    while (1) {
    }
}
