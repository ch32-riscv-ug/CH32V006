/********************************** (C) COPYRIGHT  *******************************
 * File Name          : adcsample.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2026/08/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "adcsample.h"

#define IIR_ALPHA 0.05f

typedef union {
    struct
    {
        uint32_t JSQ1 : 5;
        uint32_t JSQ2 : 5;
        uint32_t JSQ3 : 5;
        uint32_t JSQ4 : 5;
        uint32_t JL : 2;
        uint32_t Reserved : 10;
    } reg;

    uint32_t value;
} tAdcIsqr;

// AdcSample 采样
FAST_FUNC void AdcSample(bool isNeedSampleCurrent) {
    if (mcState == mcAhead) {
        ADCSample.UBemf = ADC1->IDATAR1;
        ADCSample.VBemf = ADC1->IDATAR2;
        ADCSample.WBemf = ADC1->IDATAR3;
        ADCSample.NeutralPoint = ADCSample.UBemf + ADCSample.VBemf + ADCSample.WBemf;
    } else {
        ADCSample.OnPhase = ADC1->IDATAR2;
        ADCSample.OnPhaseHalf = ADCSample.OnPhase >> 1;
        ADCSample.BemfPhase = ADC1->IDATAR3;
        if (isNeedSampleCurrent == true) {
            ADCSample.Current = ADC1->IDATAR1 - ADCSample.CurrentOffset;
            if (ADCSample.CalibrationIsOK == true) {
                ADCSample.CurrentFilter += _IQ15mpy(_IQ15(IIR_ALPHA), (ADCSample.Current - ADCSample.CurrentFilter));
            }
        }
    }

    ADCSample.Voltage = ADC1->IDATAR4;
}

// TODO 根据当前扇区自动设置ADC检测通道
FAST_FUNC void AdcSampleChannelCharge(void) {
    tAdcIsqr adcIsqr;
    adcIsqr.value = ADC1->ISQR;

    if (mcState == mcAhead) {
        adcIsqr.reg.JSQ1 = DRVADC_U_CHANNEL;
        adcIsqr.reg.JSQ2 = DRVADC_V_CHANNEL;
        adcIsqr.reg.JSQ3 = DRVADC_W_CHANNEL;
        adcIsqr.reg.JSQ4 = DRVADC_VBUS_CHANNEL;
        ADC1->ISQR = adcIsqr.value;
        return;
    }

    if (HoldParm.RotorDirection == CW) {
        switch (HoldParm.Phase) {
        case 0:
            adcIsqr.reg.JSQ2 = DRVADC_U_CHANNEL;
            adcIsqr.reg.JSQ3 = DRVADC_W_CHANNEL;
            break;
        case 1:
            adcIsqr.reg.JSQ2 = DRVADC_U_CHANNEL;
            adcIsqr.reg.JSQ3 = DRVADC_V_CHANNEL;
            break;
        case 2:
            adcIsqr.reg.JSQ2 = DRVADC_V_CHANNEL;
            adcIsqr.reg.JSQ3 = DRVADC_U_CHANNEL;
            break;
        case 3:
            adcIsqr.reg.JSQ2 = DRVADC_V_CHANNEL;
            adcIsqr.reg.JSQ3 = DRVADC_W_CHANNEL;
            break;
        case 4:
            adcIsqr.reg.JSQ2 = DRVADC_W_CHANNEL;
            adcIsqr.reg.JSQ3 = DRVADC_V_CHANNEL;
            break;
        case 5:
            adcIsqr.reg.JSQ2 = DRVADC_W_CHANNEL;
            adcIsqr.reg.JSQ3 = DRVADC_U_CHANNEL;
            break;
        default:
            break;
        }
    } else {
        switch (HoldParm.Phase) {
        case 0:
            adcIsqr.reg.JSQ2 = DRVADC_U_CHANNEL;
            adcIsqr.reg.JSQ3 = DRVADC_W_CHANNEL;
            break;
        case 1:
            adcIsqr.reg.JSQ2 = DRVADC_W_CHANNEL;
            adcIsqr.reg.JSQ3 = DRVADC_U_CHANNEL;
            break;
        case 2:
            adcIsqr.reg.JSQ2 = DRVADC_W_CHANNEL;
            adcIsqr.reg.JSQ3 = DRVADC_V_CHANNEL;
            break;
        case 3:
            adcIsqr.reg.JSQ2 = DRVADC_V_CHANNEL;
            adcIsqr.reg.JSQ3 = DRVADC_W_CHANNEL;
            break;
        case 4:
            adcIsqr.reg.JSQ2 = DRVADC_V_CHANNEL;
            adcIsqr.reg.JSQ3 = DRVADC_U_CHANNEL;
            break;
        case 5:
            adcIsqr.reg.JSQ2 = DRVADC_U_CHANNEL;
            adcIsqr.reg.JSQ3 = DRVADC_V_CHANNEL;
            break;
        default:
            break;
        }
    }

    adcIsqr.reg.JSQ1 = ADC_Channel_OPA;
    adcIsqr.reg.JSQ4 = DRVADC_VBUS_CHANNEL;

    ADC1->ISQR = adcIsqr.value;
}

// ADC触发点
FAST_FUNC void AdcTriggerPoint(uint16_t cnt) {
    TIM1->CH4CVR = cnt;
}
