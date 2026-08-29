/********************************** (C) COPYRIGHT  *******************************
 * File Name          : protect.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2026/08/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "protect.h"
#include "control.h"
#include "isr.h"

typedef struct {
    int32_t voltagMax;
    int32_t voltageMin;
    int32_t current1Max;
    int32_t current2Max;
    int32_t speedMax;
} tParam;

static tParam gParam;

#define IIR_ALPHA 0.01f // 0.25 in Q8

// ProtectInit 保护模块初始化
void ProtectInit(void) {
    gParam.voltagMax = _IQ12(OVER_VOLTAGE / VOLTAGE_MAX);
    gParam.voltageMin = _IQ12(UNDER_VOLTAGE / VOLTAGE_MAX);
    gParam.current1Max = _IQ12(OVER_CURRENT1 / CURRENT_MAX);
    gParam.current2Max = _IQ12(OVER_CURRENT2 / CURRENT_MAX);
    gParam.speedMax = 0;
}

// ProtectRun 保护模块运行
void ProtectRun(void) {
    static uint64_t gOverVoltageTime = 0;
    static uint64_t gUnderVoltageTime = 0;
    static uint64_t gOverCurrent1Time = 0;
    static uint64_t gOverCurrent2Time = 0;
    if (gOverVoltageTime == 0) {
        gOverVoltageTime = TZTimeGetMillsecond();
    }

    if (gUnderVoltageTime == 0) {
        gUnderVoltageTime = TZTimeGetMillsecond();
    }

    if (ADCSample.Voltage > gParam.voltagMax) {
        if (TZTimeGetMillsecond() - gOverVoltageTime > 200) {
            ErrorCode.bit.OverVoltage = 1;
        }
    } else {
        gOverVoltageTime = TZTimeGetMillsecond();
        ErrorCode.bit.OverVoltage = 0;
    }

    if (ADCSample.Voltage < gParam.voltageMin) {
        if (TZTimeGetMillsecond() - gUnderVoltageTime > 200) {
            ErrorCode.bit.UnderVoltage = 1;
        }
    } else {
        gUnderVoltageTime = TZTimeGetMillsecond();
        ErrorCode.bit.UnderVoltage = 0;
    }

    uint32_t current = ABS(ADCSample.CurrentFilter);

    if (current > gParam.current1Max) {
        if (TZTimeGetMillsecond() - gOverCurrent1Time > OVER_CURRENT1_TIME) {
            ErrorCode.bit.OverCurrent1 = 1;
        }
    } else {
        gOverCurrent1Time = TZTimeGetMillsecond();
    }

    if (current > gParam.current2Max) {
        if (TZTimeGetMillsecond() - gOverCurrent2Time > OVER_CURRENT2_TIME) {
            ErrorCode.bit.OverCurrent2 = 1;
        }
    } else {
        gOverCurrent2Time = TZTimeGetMillsecond();
    }

    if (ErrorCode.value != 0) {
        MotorStop();
    }
}

// ProtectRealTimeCheck 实时保护检查
void ProtectRealTimeCheck(void) {
    // 失速判断
    // 过流判断
}
