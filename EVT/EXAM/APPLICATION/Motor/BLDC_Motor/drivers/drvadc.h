/********************************** (C) COPYRIGHT  *******************************
 * File Name          : drvadc.h
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
#ifndef DRVADC_H
#define DRVADC_H

#include "def.h"

#define DRVADC_U_PIN GPIO_Pin_6
#define DRVADC_U_PORT GPIOD
#define DRVADC_U_CHANNEL ADC_Channel_6

#define DRVADC_V_PIN GPIO_Pin_5
#define DRVADC_V_PORT GPIOD
#define DRVADC_V_CHANNEL ADC_Channel_5

#define DRVADC_W_PIN GPIO_Pin_4
#define DRVADC_W_PORT GPIOD
#define DRVADC_W_CHANNEL ADC_Channel_7

#define DRVADC_VBUS_PIN GPIO_Pin_1
#define DRVADC_VBUS_PORT GPIOA
#define DRVADC_VBUS_CHANNEL ADC_Channel_1

#define DRVADC_OPA_P_PIN GPIO_Pin_7
#define DRVADC_OPA_P_PORT GPIOD
#define DRVADC_OPA_N_PIN GPIO_Pin_4
#define DRVADC_OPA_N_PORT GPIOA
#define DRVADC_OPA_CHANNEL ADC_Channel_OPA

typedef enum {
    DRVADC_OPA,
    DRVADC_VBUS,
    DRVADC_U,
    DRVADC_V,
    DRVADC_W,
    // 闭环过程中的指定相电压采样值
    DRVADC_BEMF,
} DrvAdcChannel;

// DrvAdcInit ADC初始化
void DrvAdcInit(void);

// DrvAdcGetValue ADC获取值
uint16_t DrvAdcGetValue(DrvAdcChannel channel);

// DrvAdcSetBemfChannel 设置BEMF ADC注入通道
void DrvAdcSetBemfChannel(DrvAdcChannel channel);

// DrvAdcGetVbusValue 获取母线电压
uint16_t DrvAdcGetVbusValue(void);

#endif
