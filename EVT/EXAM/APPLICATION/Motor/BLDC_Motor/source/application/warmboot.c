/********************************** (C) COPYRIGHT  *******************************
 * File Name          : warmboot.c
 * Author             : WCH
 * Version            : V1.0.1
 * Date               : 2026/11/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "warmboot.h"
#include "framework.h"

#define Max(a, b, c) ((a) > (b) ? ((a) > (c) ? (a) : (c)) : ((b) > (c) ? (b) : (c)))
#define Min(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

typedef struct {
    uint8_t lastSector;
    uint8_t sector;
    uint8_t cnt;
    bool isBemf;
    uint16_t BemfThreshold;
    uint8_t direction;
} tWarmBootPara;

static tWarmBootPara gParam = {0};

// Direction unknown: avoids requiring one extra transition to confirm CCW when the initial value defaults to CW
#define WARMBOOT_DIR_UNKNOWN 0xFF

// Sector (three-phase BEMF signs, bit0:U bit1:V bit2:W) -> the conducting phase where the zero crossing just occurred, indexed by sector value
// Derivation: every time a new sector is entered one phase BEMF crosses zero, and that crossing lies at the middle of the 60-degree conduction zone where "that phase floats",
// derived from phaseTable (AB-AC-BC-BA-CA-CB) together with the rising/falling edge definitions of edge[] in halless.c
static const uint8_t kZcPhaseCW[8] = {0, 0, 2, 1, 4, 5, 3, 0};
static const uint8_t kZcPhaseCCW[8] = {0, 1, 3, 2, 5, 0, 4, 0};

// WarmBootInit: initialize head-wind startup
void WarmBootInit(float bemfThreshold) {
    gParam.cnt = 0;
    gParam.lastSector = 0;
    gParam.sector = 0;
    gParam.direction = WARMBOOT_DIR_UNKNOWN;
    // The BEMF threshold is calculated as three times the neutral point voltage
    gParam.BemfThreshold = _IQ12(bemfThreshold * 3.0f / VOLTAGE_MAX);
}

// WarmBootCheck: check whether head/tail-wind detection sees a zero crossing
bool WarmBootCheck(void) {
    gParam.sector = 0;

    uint16_t maxBemf = Max(ADCSample.PhaseVoltage[0], ADCSample.PhaseVoltage[1], ADCSample.PhaseVoltage[2]);
    uint16_t minBemf = Min(ADCSample.PhaseVoltage[0], ADCSample.PhaseVoltage[1], ADCSample.PhaseVoltage[2]);

    // Head/tail-wind conditions
    // 1. The maximum BEMF is greater than the threshold
    // 2. The maximum BEMF is not less than half of the minimum
    if (maxBemf < gParam.BemfThreshold || (maxBemf >> 1) < minBemf) {
        gParam.isBemf = false;
        gParam.cnt = 0;
        gParam.lastSector = 0;
        return false;
    } else {
        gParam.isBemf = true;
    }

    if (ADCSample.PhaseVoltage[0] * 3 > ADCSample.NeutralPoint) {
        gParam.sector |= 1;
    }
    if (ADCSample.PhaseVoltage[1] * 3 > ADCSample.NeutralPoint) {
        gParam.sector |= 2;
    }
    if (ADCSample.PhaseVoltage[2] * 3 > ADCSample.NeutralPoint) {
        gParam.sector |= 4;
    }

    uint8_t temp = (gParam.lastSector << 4) | gParam.sector;
    gParam.lastSector = gParam.sector;

    // 5 -> 1 -> 3 -> 2 -> 6 -> 4 forward rotation
    if (temp == 0x51 || temp == 0x13 || temp == 0x32 ||
        temp == 0x26 || temp == 0x64 || temp == 0x45) {
        if (gParam.direction == CCW) {
            gParam.cnt = 0;
        } else {
            gParam.cnt++;
        }
        gParam.direction = CW;
        return true;
    }

    // 5 -> 4 -> 6 -> 2 -> 3 -> 1 reverse rotation
    if (temp == 0x54 || temp == 0x46 || temp == 0x62 ||
        temp == 0x23 || temp == 0x31 || temp == 0x15) {
        if (gParam.direction == CW) {
            gParam.cnt = 0;
        } else {
            gParam.cnt++;
        }
        gParam.direction = CCW;
        return true;
    }

    return false;
}

// WarmBootIsOK: check whether detection succeeded
bool WarmBootIsOK(void) {
    if (gParam.cnt >= 3) {
        return true;
    }
    return false;
}

// WarmBootGetSector: get the commutation sector
// According to the detected rotation direction, return the conducting phase where the zero crossing just occurred (0~5); the caller outputs it directly with PWMApplyPhase()
uint8_t WarmBootGetSector(void) {
    return (gParam.direction == CCW) ? kZcPhaseCCW[gParam.sector & 0x07] : kZcPhaseCW[gParam.sector & 0x07];
}

// WarmBootGetIsBemf: get whether BEMF has been detected
bool WarmBootGetIsBemf(void) {
    return gParam.isBemf;
}

// WarmBootReset: reset head-wind startup
void WarmBootReset(void) {
    gParam.cnt = 0;
    gParam.lastSector = 0;
    gParam.sector = 0;
    gParam.isBemf = 0;
    gParam.direction = WARMBOOT_DIR_UNKNOWN;
}

// WarmBootGetDirection: get the head-wind startup direction
// 0: forward 1: reverse 0xFF: unknown
uint8_t WarmBootGetDirection(void) {
    return gParam.direction;
}
