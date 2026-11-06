/********************************** (C) COPYRIGHT  *******************************
 * File Name          : isr.c
 * Author             : WCH
 * Version            : V1.0.1
 * Date               : 2026/11/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "isr.h"
#include "adcsample.h"
#include "control.h"
#include "framework.h"

volatile uint64_t gSysTimeUs = 0;
// Dual-frame publish buffer: only protects the timing publish ordering (so a 64-bit non-atomic write cannot be torn by preemption),
// not a sampling-period scheduling mechanism
static uint64_t gSysTimeFrames[2];
static volatile uint32_t gSysTimePublished;

void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM1_UP_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM1_BRK_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void SW_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

/*****************************************************************************
  Function name : ADC_ISR
  Description   : ADC interrupt
  Input         : none
  Output        : void
*****************************************************************************/
#ifdef ADC2
void ADC1_2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
FAST_FUNC void ADC1_2_IRQHandler(void) {
#else
void ADC1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
FAST_FUNC void ADC1_IRQHandler(void) {
#endif
    // Confirm this event first; sequences completed during handling are kept for the next interrupt.
    ADC1->STATR = ~(uint32_t)ADC_FLAG_JEOC;
    MotorControl();
}

FAST_FUNC void TIM1_UP_IRQHandler(void) {
    // Single mechanism: TIM1_UP only keeps the time accounting and publishes time frames; the ADC owns the PWM/sampling point writes

    // Control loop run interval
    // HoldParm.ControlLoopCnt++;
    // if (HoldParm.ControlLoopCnt >= CONTROL_LOOP_INTERVAL_CNT) {
    //     HoldParm.ControlLoopCnt = 0;
    //     if (NVIC_GetPendingIRQ(Software_IRQn) == RESET) {
    //         NVIC_SetPendingIRQ(Software_IRQn);
    //     }
    // }

    gSysTimeUs += PwmPeriodUs;
    // Time publish buffer: when UP is preempted by ADC0 the reader takes the last complete frame without waiting;
    // when main is preempted by UP the token check rereads, preventing ABA between two UPs on the same slot. gSysTimeUs
    // is only for UP to keep accumulating, readers only read the published frame. It does not affect PWM/sampling scheduling.
    uint32_t token = gSysTimePublished + 1u;
    gSysTimeFrames[token & 1u] = gSysTimeUs;
    __asm volatile ("" ::: "memory");
    gSysTimePublished = token;
    TIM_ClearFlag(TIM1, TIM_FLAG_Update);
}

FAST_FUNC void TIM1_BRK_IRQHandler(void) {
    if (TIM_GetITStatus(TIM1, TIM_IT_Break) != RESET) {
        MotorEmergencyStop(ERROR_MASK_OVER_CURRENT_CMP);
        // MOE is cleared automatically by the BKIN hardware; it is not re-enabled here, to avoid a race with a possibly interrupted applyPhaseRegs()
        // On restart MotorInit() re-enables MOE
        TIM_ClearFlag(TIM1, TIM_FLAG_Break);
    }
}

FAST_FUNC void SysTick_Handler(void) {
    if (HoldParm.PhaseIsChange == false) {
        if (gMotorIsNeedRun == true) {
            PWMSwitchPhase(); // Commutation
        }
    }
    SYSTICK_CMP = 0xFFFFFFFF;

    SysTick->SR &= ~(1 << 0);
    // Resume only after commutation + CMP removal + SR clear: act only when WAIT exists, a fault cancel does not revive
    MotorAdcSamplingResume();
}

// SysTimeGet: get the system time
FAST_FUNC uint64_t SysTimeGet(void) {
    // Published-frame read: an unchanged token before and after means a complete frame; two UP accumulations change the token by 2
    // and can still be distinguished (slot reuse is still detected as a retry)
    for (;;) {
        uint32_t token = gSysTimePublished;
        __asm volatile ("" ::: "memory");
        uint64_t time = gSysTimeFrames[token & 1u];
        __asm volatile ("" ::: "memory");
        if (token == gSysTimePublished) {
            return time;
        }
    }
}

/*********************************************************************
 * @fn      NMI_Handler
 *
 * @brief   This function handles NMI exception.
 *
 * @return  none
 */
void NMI_Handler(void) {
    // Turn off the bridge arms first, so PWM does not keep running when the CPU faults
    TIM_CtrlPWMOutputs(TIM1, DISABLE);
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
    // Turn off the bridge arms first, then reset the system, so PWM does not keep outputting during reset
    TIM_CtrlPWMOutputs(TIM1, DISABLE);
    NVIC_SystemReset();
    while (1) {
    }
}

FAST_FUNC void SW_Handler(void) {
    // Clear the software interrupt flag
    NVIC_ClearPendingIRQ(Software_IRQn);
}
