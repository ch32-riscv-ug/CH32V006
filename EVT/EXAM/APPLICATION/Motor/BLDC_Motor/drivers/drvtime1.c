/********************************** (C) COPYRIGHT  *******************************
 * File Name          : drvtime1.c
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

static void gpioInit(void);

// DrvTime1Init 定时器1初始化
void DrvTime1Init(void) {
    // 配置寄存器结构体定义
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {0};
    TIM_OCInitTypeDef TIM_OCInitStructure = {0};
    TIM_BDTRInitTypeDef TIM_BDTRInitStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    // 时钟配置
    RCC_PB2PeriphClockCmd(RCC_PB2Periph_GPIOA | RCC_PB2Periph_GPIOB |
                              RCC_PB2Periph_GPIOD | RCC_PB2Periph_AFIO | RCC_PB2Periph_TIM1,
                          ENABLE);

    gpioInit();

    // 定时器1配置
    TIM_DeInit(TIM1); // 定时器寄存器复位
    // 用于初始化定时器结构体的初始化
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);

    // 预分频配置
    TIM_TimeBaseStructure.TIM_Prescaler = 0; /*48MHz */

    // 计数模式配置——向上计数模式
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; /*向上计数*/

    // 周期配置
    TIM_TimeBaseStructure.TIM_Period = PWM_FRE_SETATA; /*2400  开关频率为20K*/

    // 数字滤波器和死区发生器分频比例：2分频
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV2; /*Tdts = 2*Tck_int*/ //---补充：后续放置到4倍---

    // 事件更新速率
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;

    // 配置寄存器实体
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    // 比较器初始化用结构体的初始化
    TIM_OCStructInit(&TIM_OCInitStructure);

    // 小于比较寄存器值为有效，否则为无效
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;

    // 正向通道比较输出使能
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Disable;

    // 互补通道比较输出不使能       //---补充：互补通道在大电流应用需要打开---
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;

    // dummy value
    TIM_OCInitStructure.TIM_Pulse = 0;

    // 正向通道比较输出高电平有效
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

    // 互补通道比较输出高电平有效
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;

    // 正向通道空闲状态为低电平
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;

    // 互补通道空闲状态为低电平
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset; // PU预驱

    TIM_OC1Init(TIM1, &TIM_OCInitStructure); // 捕获比较通道1初始化
    TIM_OC2Init(TIM1, &TIM_OCInitStructure); // 捕获比较通道2初始化
    TIM_OC3Init(TIM1, &TIM_OCInitStructure); // 捕获比较通道3初始化

    // 通道4配置，用于触发ADC
    TIM_OCStructInit(&TIM_OCInitStructure);

    // 大于比较寄存器值为有效，否则为无效
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;

    // 正向通道比较输出使能
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;

    // 互补通道比较输出禁止
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;

    // 捕获比较寄存器幅值为定时器周期值-1
    TIM_OCInitStructure.TIM_Pulse = 0;

    // 正通道高比较输出高电平有效
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

    // 互补通道比较输出低电平有效
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_Low;

    // 互补通道空闲状态为低电平
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;

    // 互补通道空闲状态低为电平
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Reset;

    TIM_OC4Init(TIM1, &TIM_OCInitStructure); // 捕获比较通道4初始化

    // 四路比较输出的预加载配置
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Disable); // 不使能捕获比较通道1的预装载
    TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Disable); // 不使能捕获比较通道1的预装载
    TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Disable); // 不使能捕获比较通道1的预装载
    TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Disable); // 不使能捕获比较通道4的预装载

    // 刹车死区相关配置
    TIM_BDTRInitStructure.TIM_OSSRState = TIM_OSSRState_Enable; // OSSR使能
    TIM_BDTRInitStructure.TIM_OSSIState = TIM_OSSIState_Enable; // OSSI使能

    // 寄存器配置锁定级别为1
    TIM_BDTRInitStructure.TIM_LOCKLevel = TIM_LOCKLevel_1;

    // 设置死区时间
    TIM_BDTRInitStructure.TIM_DeadTime = DRVTIME1_DEADTIME_CONFIGURE;

    TIM_BDTRInitStructure.TIM_Break = TIM_Break_Enable; // 刹车使能

    // 刹车信号极性配置为高有效
    TIM_BDTRInitStructure.TIM_BreakPolarity = TIM_BreakPolarity_High;

    // 自动输出禁止
    TIM_BDTRInitStructure.TIM_AutomaticOutput = TIM_AutomaticOutput_Disable;

    // 配置寄存器实体
    TIM_BDTRConfig(TIM1, &TIM_BDTRInitStructure);

    // 中断配置
    TIM_CtrlPWMOutputs(TIM1, ENABLE);
    TIM_ARRPreloadConfig(TIM1, ENABLE);
    TIM_SelectOutputTrigger(TIM1, TIM_TRGOSource_Update); // 选择更新事件作为外部触发源
    TIM_SetCounter(TIM1, 0);                              // 定时器计数器清零

    // Timer1中断配置
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);           // 中断优先级分组配置
    NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;        // 中断通道TIM1_UP_IRQn
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // 抢占优先级0
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;        // 从优先级2
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           // 中断使能
    NVIC_Init(&NVIC_InitStructure);                           // 配置寄存器实体

    // 刹车中断配置
    NVIC_InitStructure.NVIC_IRQChannel = TIM1_BRK_IRQn;       // 中断通道TIM1_BRK_IRQn
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // 抢占优先级0
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;        // 从优先级1
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           // 中断使能
    NVIC_Init(&NVIC_InitStructure);                           // 配置寄存器实体
    TIM_ClearITPendingBit(TIM1, TIM_IT_Break);                // 清除刹车中断标志清除
    TIM_ITConfig(TIM1, TIM_IT_Break | TIM_IT_Update, ENABLE); // 使能刹车中断

    TIM_Cmd(TIM1, ENABLE);
}

static void gpioInit(void) {
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_PinRemapConfig(GPIO_PartialRemap6_TIM1, ENABLE);
    // PWM用IO配置
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = DRVTIME1_UH_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // 上管PWM
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = DRVTIME1_VH_PIN | DRVTIME1_WH_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // 上管PWM
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // 下管PWM

    GPIO_InitStructure.GPIO_Pin = DRVTIME1_UL_PIN | DRVTIME1_VL_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure); // 配置寄存器实体
    GPIO_ResetBits(GPIOA, DRVTIME1_UL_PIN | DRVTIME1_VL_PIN);

    GPIO_InitStructure.GPIO_Pin = DRVTIME1_WL_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure); // 配置寄存器实体
    GPIO_ResetBits(GPIOD, DRVTIME1_WL_PIN);
}

// DrvTime1SetHighBridgePwm 设置高桥PWM占空比
void DrvTime1SetHighBridgePwm(DRVTIME1_PHASE phase, uint16_t period) {
    switch (phase) {
    case DRVTIME1_PHASE_U:
        TIM1->CH1CVR = period;
        break;
    case DRVTIME1_PHASE_V:
        TIM1->CH2CVR = period;
        break;
    case DRVTIME1_PHASE_W:
        TIM1->CH3CVR = period;
        break;
    default:
        break;
    }
}

// DrvTime1SetLowBridgeSwitch 设置低桥开关
void DrvTime1SetLowBridgeSwitch(DRVTIME1_PHASE phase, bool state) {
    switch (phase) {
    case DRVTIME1_PHASE_U:
        if (state) {
            GPIO_SetBits(DRVTIME1_UL_PORT, DRVTIME1_UL_PIN);
        } else {
            GPIO_ResetBits(DRVTIME1_UL_PORT, DRVTIME1_UL_PIN);
        }
        break;
    case DRVTIME1_PHASE_V:
        if (state) {
            GPIO_SetBits(DRVTIME1_VL_PORT, DRVTIME1_VL_PIN);
        } else {
            GPIO_ResetBits(DRVTIME1_VL_PORT, DRVTIME1_VL_PIN);
        }
        break;
    case DRVTIME1_PHASE_W:
        if (state) {
            GPIO_SetBits(DRVTIME1_WL_PORT, DRVTIME1_WL_PIN);
        } else {
            GPIO_ResetBits(DRVTIME1_WL_PORT, DRVTIME1_WL_PIN);
        }
        break;
    default:
        break;
    }
}

// DrvTime1SetAllLowBridgeSwitch 设置所有低桥开关
void DrvTime1SetAllLowBridgeSwitch(bool state) {
    if (state) {
        GPIO_SetBits(DRVTIME1_UL_PORT, DRVTIME1_UL_PIN);
        GPIO_SetBits(DRVTIME1_VL_PORT, DRVTIME1_VL_PIN);
        GPIO_SetBits(DRVTIME1_WL_PORT, DRVTIME1_WL_PIN);
    } else {
        GPIO_ResetBits(DRVTIME1_UL_PORT, DRVTIME1_UL_PIN);
        GPIO_ResetBits(DRVTIME1_VL_PORT, DRVTIME1_VL_PIN);
        GPIO_ResetBits(DRVTIME1_WL_PORT, DRVTIME1_WL_PIN);
    }
}

// DrvTime1SetCC4 设置CC4占空比
void DrvTime1SetCC4(uint16_t duty) {
    TIM1->CH4CVR = duty;
}

// DrvTime1SetAllOff 设置所有输出为低电平
void DrvTime1SetAllOff(void) {
    DrvTime1SetLowBridgeSwitch(DRVTIME1_PHASE_U, false);
    DrvTime1SetLowBridgeSwitch(DRVTIME1_PHASE_V, false);
    DrvTime1SetLowBridgeSwitch(DRVTIME1_PHASE_W, false);
    DrvTime1SetHighBridgePwm(DRVTIME1_PHASE_U, 0);
    DrvTime1SetHighBridgePwm(DRVTIME1_PHASE_V, 0);
    DrvTime1SetHighBridgePwm(DRVTIME1_PHASE_W, 0);
}

void DrvTime1Start(void) {
    TIM_Cmd(TIM1, ENABLE);
}

void DrvTime1Stop(void) {
    TIM_Cmd(TIM1, DISABLE);
}
