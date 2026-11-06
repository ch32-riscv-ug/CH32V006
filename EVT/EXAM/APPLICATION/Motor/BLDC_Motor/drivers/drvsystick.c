/********************************** (C) COPYRIGHT  *******************************
 * File Name          : drvsystick.c
 * Author             : WCH
 * Version            : V1.0.1
 * Date               : 2026/11/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "drivers.h"

// DrvSystickInit: driver initialization
void DrvSystickInit(void) {
    NVIC_InitTypeDef NVIC_InitStructure = {0};
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1); // Interrupt priority grouping configuration

    SysTick->CTLR = 0;
    SysTick->CMP = 0;
    SysTick->SR &= ~(1 << 0); // Clear the flag bit
    SysTick->CNT = 0;
    // CMP is initialized to "never match" so that during mcAhead (head/tail-wind
    // detection), when nobody re-arms CMP, this stale default value cannot trigger
    // a premature commutation: the tick count of STALL_TIME is smaller than
    // WARMBOOT_TIMEOUT, so SysTick_Handler would force the phase under test low
    // early during mcAhead and corrupt the three-phase BEMF readings of wind
    // detection. Commutation delay (delayCharge()) and the stall fallback in the
    // mcRun stage both explicitly re-arm CMP and do not rely on the value here.
    SysTick->CMP = 0xFFFFFFFF;
    // Enable interrupt, no prescaler
    SysTick->CTLR |= (1 << 1) | (1 << 2);

    NVIC_InitStructure.NVIC_IRQChannel = SysTicK_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // Preemption priority 0
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;        // Sub priority 0
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

// DrvSystickCmd: enable or disable the systick timer
// enable: enable flag
// CNT is set to 0 whether enabled or disabled
void DrvSystickCmd(bool enable) {
    if (enable == true && (SysTick->CTLR & TIM_CEN) == 0) {
        SysTick->CTLR |= TIM_CEN;
    }

    if (enable == false && (SysTick->CTLR & TIM_CEN) != 0) {
        SysTick->CTLR &= (uint16_t)(~((uint16_t)TIM_CEN));
    }

    SysTick->CNT = 0;
}

// DrvSystickClear: clear the systick counter
void DrvSystickClear(void) {
    SysTick->CNT = 0;
}
