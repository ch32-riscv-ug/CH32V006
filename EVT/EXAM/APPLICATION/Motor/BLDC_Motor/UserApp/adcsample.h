/********************************** (C) COPYRIGHT  *******************************
 * File Name          : adcsample.h
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
#ifndef ADCSAMPLE_H
#define ADCSAMPLE_H

#include "drivers.h"
#include "common.h"

// ADC采样
FAST_FUNC void AdcSample(bool isNeedSampleCurrent);

// ADC采样通道切换
FAST_FUNC void AdcSampleChannelCharge(void);

// ADC触发点
FAST_FUNC void AdcTriggerPoint(uint16_t cnt);

#endif

