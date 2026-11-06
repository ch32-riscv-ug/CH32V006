/********************************** (C) COPYRIGHT  *******************************
 * File Name          : drvadc.h
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
#ifndef DRVADC_H
#define DRVADC_H

// Definitions required to export common types and sampling pin macros
#include "def.h" // IWYU pragma: export

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

typedef struct {
    uint16_t current;
    uint16_t voltage;
} DrvAdcRegularSample_T;

typedef struct {
    uint32_t frameCount;
    uint32_t missedCount;
    uint32_t dmaErrorCount;
    uint16_t currentRaw;
    uint16_t voltageRaw;
} DrvAdcDiag_T;

extern volatile DrvAdcDiag_T gDrvAdcDiag;

// DrvAdcInit: ADC initialization
void DrvAdcInit(void);

// Read the regular group sample directly, allowed across rounds, without disabling interrupts or clearing DMA flags
bool DrvAdcGetRegularSample(DrvAdcRegularSample_T *sample);

// Update readings and DMA progress from the control interrupt, may repeat old values, without stopping or re-arming DMA
bool DrvAdcReadRegular(DrvAdcRegularSample_T *sample);

// Pause the regular group during static pulse detection and restore the three-phase sampling chain on exit
void DrvAdcPauseRegular(void);
void DrvAdcResumeRegular(void);

// DrvAdcGetVbusValue: get the bus voltage
uint16_t DrvAdcGetVbusValue(void);

#endif
