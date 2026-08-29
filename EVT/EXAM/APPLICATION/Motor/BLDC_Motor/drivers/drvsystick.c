/********************************** (C) COPYRIGHT  *******************************
 * File Name          : drvsystick.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2026/08/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "drivers.h"

// DrvSystickInit 驱动初始化
void DrvSystickInit(void) {
    NVIC_InitTypeDef NVIC_InitStructure = {0};
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1); // 中断优先级分组配置

    SysTick->CTLR = 0;
    SysTick->CMP = 0;
    SysTick->SR &= ~(1 << 0); // 清除标志位
    SysTick->CNT = 0;
    SysTick->CMP = STALL_TIME * (SYSTEM_CLOCK_FREQ / 1000);
    // 使能中断 不分频
    SysTick->CTLR |= (1 << 1) | (1 << 2);

    NVIC_InitStructure.NVIC_IRQChannel = SysTicK_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // 抢占优先级为1
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;        // 从优先级为3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

// DrvSystickCmd 使能或失能滴答时钟
// enable 使能
// 无论使能还是使能都会设置CNT为0
void DrvSystickCmd(bool enable) {
    if (enable == true && (SysTick->CTLR & TIM_CEN) == 0) {
        SysTick->CTLR |= TIM_CEN;
    }

    if (enable == false && (SysTick->CTLR & TIM_CEN) != 0) {
        SysTick->CTLR &= (uint16_t)(~((uint16_t)TIM_CEN));
    }

    SysTick->CNT = 0;
}

// DrvSystickClear 清除滴答计数
void DrvSystickClear(void) {
    SysTick->CNT = 0;
}
