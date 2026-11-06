/********************************** (C) COPYRIGHT  *******************************
 * File Name          : protect.c
 * Author             : WCH
 * Version            : V1.0.1
 * Date               : 2026/11/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#include "control.h"
#include "framework.h"

// Compatible default: degrade to "recover immediately" (no hysteresis) when no recovery threshold is configured
#ifndef OVER_VOLTAGE_RECOVER
#define OVER_VOLTAGE_RECOVER OVER_VOLTAGE
#endif
#ifndef UNDER_VOLTAGE_RECOVER
#define UNDER_VOLTAGE_RECOVER UNDER_VOLTAGE
#endif
#ifndef VOLTAGE_PROTECT_TIME
#define VOLTAGE_PROTECT_TIME 200
#endif
// Faults do not perform uncontrolled winding short-circuit braking; stall still allows the original drag recovery.
#define STOP_FAULT_MASK ERROR_MASK_BLOCKING

typedef struct {
    uint32_t voltagMax;           // Over-voltage trigger threshold
    uint32_t voltageMin;          // Under-voltage trigger threshold
    uint32_t voltageOverRecover;  // Over-voltage recovery threshold
    uint32_t voltageUnderRecover; // Under-voltage recovery threshold
    uint32_t current1Max;
    uint32_t current2Max;
    uint32_t speedMax;
} tParam;

static tParam gParam;

static bool gIsFrist = true;

void ProtectSetError(uint16_t errorMask) {
    uint32_t mstatus = criticalEnter();
    ErrorCode.value |= errorMask;
    criticalExit(mstatus);
}

void ProtectClearErrorMask(uint16_t errorMask) {
    uint32_t mstatus = criticalEnter();
    ErrorCode.value &= (uint16_t)~errorMask;
    criticalExit(mstatus);
}

// ProtectInit: protection module initialization
void ProtectInit(void) {
    gParam.voltagMax = _IQ12(OVER_VOLTAGE / VOLTAGE_MAX);
    gParam.voltageMin = _IQ12(UNDER_VOLTAGE / VOLTAGE_MAX);
    gParam.voltageOverRecover = _IQ12(OVER_VOLTAGE_RECOVER / VOLTAGE_MAX);
    gParam.voltageUnderRecover = _IQ12(UNDER_VOLTAGE_RECOVER / VOLTAGE_MAX);
    gParam.current1Max = _IQ12(OVER_CURRENT1 / CURRENT_MAX);
    gParam.current2Max = _IQ12(OVER_CURRENT2 / CURRENT_MAX);
    gParam.speedMax = 0;
}

// ProtectRun: protection module run
void ProtectRun(void) {
    static uint64_t gOverVoltageTime = 0;
    static uint64_t gUnderVoltageTime = 0;
    static uint64_t gOverCurrent1Time = 0;
    static uint64_t gOverCurrent2Time = 0;
    if (gIsFrist == true) {
        gOverVoltageTime = TZTimeGetMillsecond();
        gUnderVoltageTime = TZTimeGetMillsecond();
        gOverCurrent1Time = TZTimeGetMillsecond();
        gOverCurrent2Time = TZTimeGetMillsecond();
        gIsFrist = false;
    }

    // Over-voltage protection (hysteresis): after triggering, the voltage must stay below the recovery threshold for VOLTAGE_PROTECT_TIME before it is cleared
    if (ErrorCode.bit.OverVoltage == 0) {
        if (ADCSample.Voltage > gParam.voltagMax) {
            if (TZTimeGetMillsecond() - gOverVoltageTime > VOLTAGE_PROTECT_TIME) {
                ProtectSetError(ERROR_MASK_OVER_VOLTAGE);
                gOverVoltageTime = TZTimeGetMillsecond();
            }
        } else {
            gOverVoltageTime = TZTimeGetMillsecond();
        }
    } else {
        if (ADCSample.Voltage < gParam.voltageOverRecover) {
            if (TZTimeGetMillsecond() - gOverVoltageTime > VOLTAGE_PROTECT_TIME) {
                ProtectClearErrorMask(ERROR_MASK_OVER_VOLTAGE);
                gOverVoltageTime = TZTimeGetMillsecond();
            }
        } else {
            gOverVoltageTime = TZTimeGetMillsecond();
        }
    }

    // Under-voltage protection (hysteresis): after triggering, the voltage must stay above the recovery threshold for VOLTAGE_PROTECT_TIME before it is cleared
    if (ErrorCode.bit.UnderVoltage == 0) {
        if (ADCSample.Voltage < gParam.voltageMin) {
            if (TZTimeGetMillsecond() - gUnderVoltageTime > VOLTAGE_PROTECT_TIME) {
                ProtectSetError(ERROR_MASK_UNDER_VOLTAGE);
                gUnderVoltageTime = TZTimeGetMillsecond();
            }
        } else {
            gUnderVoltageTime = TZTimeGetMillsecond();
        }
    } else {
        if (ADCSample.Voltage > gParam.voltageUnderRecover) {
            if (TZTimeGetMillsecond() - gUnderVoltageTime > VOLTAGE_PROTECT_TIME) {
                ProtectClearErrorMask(ERROR_MASK_UNDER_VOLTAGE);
                gUnderVoltageTime = TZTimeGetMillsecond();
            }
        } else {
            gUnderVoltageTime = TZTimeGetMillsecond();
        }
    }

    uint32_t current = ABS(ADCSample.CurrentFilter);

    if (current > gParam.current1Max) {
        if (TZTimeGetMillsecond() - gOverCurrent1Time > OVER_CURRENT1_TIME) {
            ProtectSetError(ERROR_MASK_OVER_CURRENT1);
        }
    } else {
        gOverCurrent1Time = TZTimeGetMillsecond();
    }

    if (current > gParam.current2Max) {
        if (TZTimeGetMillsecond() - gOverCurrent2Time > OVER_CURRENT2_TIME) {
            ProtectSetError(ERROR_MASK_OVER_CURRENT2);
        }
    } else {
        gOverCurrent2Time = TZTimeGetMillsecond();
    }

    // if (ADCSample.Ntc < 30) {
    //     ErrorCode.bit.TempOver = 1;
    // } else if (ADCSample.Ntc > 61) {
    //     ErrorCode.bit.TempOver = 0;
    // }

    ProtectRealTimeCheck();
}

// A continuous fault forbids re-driving, not just the fault edge; it is not executed repeatedly after shutdown.
void ProtectRealTimeCheck(void) {
    uint16_t cur = ErrorCode.value;

    if ((cur & STOP_FAULT_MASK) != 0) {
        if (gMotorIsNeedRun || (mcState != mcIdle && mcState != mcStop)) {
            MotorEmergencyStop(0);
        }
    }
}

// ProtectClearError: clear the error code
void ProtectClearError(void) {
    uint32_t mstatus = criticalEnter();
    ErrorCode.value = 0;
    criticalExit(mstatus);
}
