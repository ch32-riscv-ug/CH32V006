/********************************** (C) COPYRIGHT  *******************************
 * File Name          : drvtime1.h
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2026/08/05
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

// PWM引脚配置
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

//死区寄存器配置值
#define DRVTIME1_DEADTIME_CONFIGURE  (uint8_t)24

typedef enum {
    DRVTIME1_PHASE_U,
    DRVTIME1_PHASE_V,
    DRVTIME1_PHASE_W,
} DRVTIME1_PHASE;

// DrvTime1Init 定时器1初始化
void DrvTime1Init(void);

// DrvTime1SetHighBridgePwm 设置高桥PWM占空比
void DrvTime1SetHighBridgePwm(DRVTIME1_PHASE phase, uint16_t period);

// DrvTime1SetLowBridgeSwitch 设置低桥开关
void DrvTime1SetLowBridgeSwitch(DRVTIME1_PHASE phase, bool state);

// DrvTime1SetAllLowBridgeSwitch 设置所有低桥开关
void DrvTime1SetAllLowBridgeSwitch(bool state);

// DrvTime1SetAllOff 设置所有输出为低电平
void DrvTime1SetAllOff(void);

// DrvTime1Start 启动定时器
void DrvTime1Start(void);

// DrvTime1Stop 停止定时器
void DrvTime1Stop(void);

// DrvTime1SetCC4 设置CC4占空比
void DrvTime1SetCC4(uint16_t duty);

#endif
