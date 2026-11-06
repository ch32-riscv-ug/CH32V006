/********************************** (C) COPYRIGHT  *******************************
 * File Name          : drvtime1.c
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
#include "pwm.h"

static void gpioInit(void);

// DrvTime1Init: timer 1 initialization
void DrvTime1Init(void) {
    // Configuration register structure definition
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {0};
    TIM_OCInitTypeDef TIM_OCInitStructure = {0};
    TIM_BDTRInitTypeDef TIM_BDTRInitStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    // Clock configuration
    RCC_PB2PeriphClockCmd(RCC_PB2Periph_GPIOA | RCC_PB2Periph_GPIOB |
                              RCC_PB2Periph_GPIOD | RCC_PB2Periph_AFIO | RCC_PB2Periph_TIM1,
                          ENABLE);

    RCC_PB1PeriphClockCmd(RCC_PB1Periph_TIM3, ENABLE);
    gpioInit();

    // Timer 1 configuration
    TIM_DeInit(TIM1); // Reset timer registers
    // Initialization of the structure used to initialize the timer
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);

    // Prescaler configuration
    TIM_TimeBaseStructure.TIM_Prescaler = 0; /*48MHz */

    // Counter mode configuration - up counting mode
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; /*Up counting*/

    // Period configuration
    TIM_TimeBaseStructure.TIM_Period = PWM_FRE_SETATA; /*2400, switching frequency 20K*/

    // Digital filter and dead-time generator division ratio: divide by 2
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV2; /*Tdts = 2*Tck_int*/ // TODO: move to divide by 4 later

    // Event update rate
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;

    // Configuration register instance
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    // Initialization of the structure used for comparator initialization
    TIM_OCStructInit(&TIM_OCInitStructure);

    // Active when smaller than the compare register value, inactive otherwise
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;

    // Forward channel compare output enable
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Disable;

    // Complementary channel compare output disabled      // TODO: complementary channel must be enabled in high-current applications
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;

    // dummy value
    TIM_OCInitStructure.TIM_Pulse = 0;

    // Forward channel compare output active high
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

    // Complementary channel compare output active high
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;

    // Forward channel idle state low
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;

    // Complementary channel idle state low
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset; // PU pre-driver

    TIM_OC1Init(TIM1, &TIM_OCInitStructure); // Capture/compare channel 1 initialization
    TIM_OC2Init(TIM1, &TIM_OCInitStructure); // Capture/compare channel 2 initialization
    TIM_OC3Init(TIM1, &TIM_OCInitStructure); // Capture/compare channel 3 initialization

    // The ADC injected trigger is handled by TIM3_CC1 instead; this board has no CMP zero-crossing input, so TIM1_CH4 is not configured.
    // Preload configuration of the three PWM compare outputs
    // Under the ADC direct-PWM mechanism the CCR writes must take effect immediately, so preload is disabled for all three compare outputs
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Disable); // Disable preload of capture/compare channel 1
    TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Disable); // Disable preload of capture/compare channel 2
    TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Disable); // Disable preload of capture/compare channel 3

    // Break and dead-time related configuration
    TIM_BDTRInitStructure.TIM_OSSRState = TIM_OSSRState_Enable; // OSSR enable
    TIM_BDTRInitStructure.TIM_OSSIState = TIM_OSSIState_Enable; // OSSI enable

    // Register configuration lock level 1
    TIM_BDTRInitStructure.TIM_LOCKLevel = TIM_LOCKLevel_1;

    // Set the dead time
    TIM_BDTRInitStructure.TIM_DeadTime = DRVTIME1_DEADTIME_CONFIGURE;

    TIM_BDTRInitStructure.TIM_Break = TIM_Break_Enable; // Break enable

    // Break signal polarity configured as active high
    TIM_BDTRInitStructure.TIM_BreakPolarity = TIM_BreakPolarity_High;

    // Automatic output disabled
    TIM_BDTRInitStructure.TIM_AutomaticOutput = TIM_AutomaticOutput_Disable;

    // Configuration register instance
    TIM_BDTRConfig(TIM1, &TIM_BDTRInitStructure);

    // Interrupt configuration
    TIM_CtrlPWMOutputs(TIM1, ENABLE);
    TIM_ARRPreloadConfig(TIM1, ENABLE);
    TIM_SelectOutputTrigger(TIM1, TIM_TRGOSource_Update); // Select the update event as external trigger source
    TIM_SetCounter(TIM1, 0);                              // Clear the timer counter
    // TIM3 slave mode: TIM1 update event (ITR0) resets the TIM3 counter, and the TIM3_CH1 compare event serves as the ADC injected trigger;
    // TIM3 compare has no preload, so the shared library writes TIM3->TIM3_CH1CVR via ADC_TRIGGER_POINT to update the sampling point immediately
    TIM_ITRxExternalClockConfig(TIM3, TIM_TS_ITR0);
    TIM_SelectSlaveMode(TIM3, TIM_SlaveMode_Reset);

    TIM_CounterModeConfig(TIM3, TIM_CounterMode_Up);
    TIM_SetAutoreload(TIM3, 65535);

    // Timer1 interrupt configuration
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);           // Interrupt priority grouping configuration
    // TIM1_UP only keeps the time accounting and publishes the safety time frame, it no longer loads sampling points/duty cycles,
    // and its preemption priority is lower than ADC (0), allowing the ADC JEOC interrupt to preempt it
    NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;        // Interrupt channel TIM1_UP_IRQn
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // Preemption priority 1, lower than ADC
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;        // Sub priority 0
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           // Interrupt enable
    NVIC_Init(&NVIC_InitStructure);                           // Configuration register instance

    // Break interrupt configuration
    NVIC_InitStructure.NVIC_IRQChannel = TIM1_BRK_IRQn;       // Interrupt channel TIM1_BRK_IRQn
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // Preemption priority 0
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;        // Sub priority 0
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           // Interrupt enable
    NVIC_Init(&NVIC_InitStructure);                           // Configuration register instance
    TIM_ClearITPendingBit(TIM1, TIM_IT_Break);                // Clear the break interrupt flag
    TIM_ITConfig(TIM1, TIM_IT_Break | TIM_IT_Update, ENABLE); // Enable the break interrupt

    PwmPeriod = PWM_FRE_SETATA;

    TIM_Cmd(TIM1, ENABLE);
    TIM_Cmd(TIM3, ENABLE);
}

static void gpioInit(void) {
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_PinRemapConfig(GPIO_PartialRemap6_TIM1, ENABLE);
    // IO configuration for PWM
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = DRVTIME1_UH_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // High-side PWM
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = DRVTIME1_VH_PIN | DRVTIME1_WH_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // High-side PWM
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // Low-side PWM

    GPIO_InitStructure.GPIO_Pin = DRVTIME1_UL_PIN | DRVTIME1_VL_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure); // Configuration register instance
    GPIO_ResetBits(GPIOA, DRVTIME1_UL_PIN | DRVTIME1_VL_PIN);

    GPIO_InitStructure.GPIO_Pin = DRVTIME1_WL_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure); // Configuration register instance
    GPIO_ResetBits(GPIOD, DRVTIME1_WL_PIN);
}

// DrvTime1SetHighBridgePwm: set the high-side bridge PWM duty cycle
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

// DrvTime1SetLowBridgeSwitch: set a low-side bridge switch
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

// DrvTime1SetAllLowBridgeSwitch: set all low-side bridge switches
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

// DrvTime1SetCC4: set the CC4 duty cycle
void DrvTime1SetCC4(uint16_t duty) {
    TIM1->CH4CVR = duty;
}

// DrvTime1SetAllOff: set all outputs low
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
