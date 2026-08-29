/********************************** (C) COPYRIGHT  *******************************
 * File Name          : warmboot.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2026/08/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "warmboot.h"
#include "common.h"

#define Max(a, b, c) ((a) > (b) ? ((a) > (c) ? (a) : (c)) : (b))
#define Min(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : (b))

typedef struct {
    uint8_t lastSector;
    uint8_t sector;
    uint8_t cnt;
    bool isBemf;
    uint16_t BemfThreshold;
    uint8_t direction;
} tWarmBootPara;

static tWarmBootPara gParam = {0};

// WarmBootInit 初始化顺风启动
void WarmBootInit(float bemfThreshold) {
    gParam.cnt = 0;
    gParam.lastSector = 0;
    gParam.sector = 0;
    // BEMF阈值 计算的是中性点电压的三倍
    gParam.BemfThreshold = _IQ12(bemfThreshold * 3.0f / VOLTAGE_MAX);
}

// WarmBootCheck 顺逆风检测是否过零
bool WarmBootCheck(void) {
    gParam.sector = 0;

    uint16_t maxBemf = Max(ADCSample.UBemf, ADCSample.VBemf, ADCSample.WBemf);
    uint16_t minBemf = Min(ADCSample.UBemf, ADCSample.VBemf, ADCSample.WBemf);

    // 顺逆风条件
    // 1. BEMF最大值大于阈值
    // 2. BEMF最大值不小于最小值的一半
    if (maxBemf < gParam.BemfThreshold || (maxBemf >> 1) < minBemf) {
        gParam.isBemf = false;
        return false;
    } else {
        gParam.isBemf = true;
    }

    if (ADCSample.UBemf * 3 > ADCSample.NeutralPoint) {
        gParam.sector |= 1;
    }
    if (ADCSample.VBemf * 3 > ADCSample.NeutralPoint) {
        gParam.sector |= 2;
    }
    if (ADCSample.WBemf * 3 > ADCSample.NeutralPoint) {
        gParam.sector |= 4;
    }

    uint8_t temp = (gParam.lastSector << 4) | gParam.sector;
    gParam.lastSector = gParam.sector;

    // 5 -> 1 -> 3 -> 2 -> 6 -> 4 正转
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

    // 5 -> 4 -> 6 -> 2 -> 3 -> 1 反转
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

// WarmBootIsOK 检测是否检测到
bool WarmBootIsOK(void) {
    if (gParam.cnt >= 3) {
        return true;
    }
    return false;
}

// WarmBootGetSector 获取换相扇区
uint8_t WarmBootGetSector(void) {
    switch (gParam.sector) {
    case 1:
        return 1;
    case 3:
        return 2;
    case 2:
        return 3;
    case 6:
        return 4;
    case 4:
        return 5;
    case 5:
        return 0;
    default:
        return 0;
    }

    return 0;
}

// WarmBootGetIsBemf 获取是否检测到BEMF
bool WarmBootGetIsBemf(void) {
    return gParam.isBemf;
}

// WarmBootReset 重置顺风启动
void WarmBootReset(void) {
    gParam.cnt = 0;
    gParam.lastSector = 0;
    gParam.sector = 0;
}

// WarmBootGetDirection 获取顺风启动方向
// 0: 正转 1: 反转
uint8_t WarmBootGetDirection(void) {
    return gParam.direction;
}
