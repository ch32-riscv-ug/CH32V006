/********************************** (C) COPYRIGHT  *******************************
 * File Name          : drvtime1.h
 * Author             : WCH
 * Version            : V1.0.1
 * Date               : 2026/11/05
 * Description        : This file contains all the functions prototypes for the
 *                       firmware library.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef DRVTIME1_H
#define DRVTIME1_H

#include "def.h"

// PWM pin configuration
#define DRVTIME1_UH_PORT GPIOA
#define DRVTIME1_UH_PIN GPIO_Pin_3
#define DRVTIME1_VH_PORT GPIOB
#define DRVTIME1_VH_PIN GPIO_Pin_0
#define DRVTIME1_WH_PORT GPIOB
#define DRVTIME1_WH_PIN GPIO_Pin_1

#define DRVTIME1_UL_PORT GPIOA
#define DRVTIME1_UL_PIN GPIO_Pin_0
#define DRVTIME1_VL_PORT GPIOA
#define DRVTIME1_VL_PIN GPIO_Pin_2
#define DRVTIME1_WL_PORT GPIOD
#define DRVTIME1_WL_PIN GPIO_Pin_0

// Dead-time register configuration value
#define DRVTIME1_DEADTIME_CONFIGURE  (uint8_t)24

typedef enum {
    DRVTIME1_PHASE_U,
    DRVTIME1_PHASE_V,
    DRVTIME1_PHASE_W,
} DRVTIME1_PHASE;

// DrvTime1Init: timer 1 initialization
void DrvTime1Init(void);

// DrvTime1SetHighBridgePwm: set the high-side bridge PWM duty cycle
void DrvTime1SetHighBridgePwm(DRVTIME1_PHASE phase, uint16_t period);

// DrvTime1SetLowBridgeSwitch: set a low-side bridge switch
void DrvTime1SetLowBridgeSwitch(DRVTIME1_PHASE phase, bool state);

// DrvTime1SetAllLowBridgeSwitch: set all low-side bridge switches
void DrvTime1SetAllLowBridgeSwitch(bool state);

// DrvTime1SetAllOff: set all outputs low
void DrvTime1SetAllOff(void);

// DrvTime1Start: start the timer
void DrvTime1Start(void);

// DrvTime1Stop: stop the timer
void DrvTime1Stop(void);

// DrvTime1SetCC4: set the CC4 duty cycle
void DrvTime1SetCC4(uint16_t duty);

#endif
