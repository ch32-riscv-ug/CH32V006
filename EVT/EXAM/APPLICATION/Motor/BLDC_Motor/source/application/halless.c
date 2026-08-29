/********************************** (C) COPYRIGHT  *******************************
 * File Name          : halless.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2026/08/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "halless.h"
#include "common.h"
#include "control.h"
#include "pwm.h"

#define FILTER_OFF_COUNT 12
#define FILTER_ON_COUNT 24

// Q15 0.95
#define BEMF_MAX_COFF 31129
// Q15 0.05
#define BEMF_MIN_COFF 1638

/*AND & OR 用于屏蔽有效BEMF信号的运算符*/
/*与运算只获取当前要检测反电动势的状态， 通过异或检测当前反电动势变化情况*/
const uint8_t ADC_MASK[2][6] = {1, 0, 1, 0, 1, 0,  // 正转
                                0, 1, 0, 1, 0, 1}; // 反转

/*BEMF 择多函数滤波*/
/*通过检测多次，当捕获到三个反电动势边沿变化中，有两个有效信号则滤波完成*/
const uint8_t ADC_BEMF_FILTER[64] =
    // 0    1    2    3      4     5     6     7     8     9     10    11    12    13    14    15
    {0x00, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E, 0x10, 0x12, 0x14, 0x16, 0x18, 0x1A, 0x1C, 0x1E,
     0x20, 0x22, 0x24, 0x26, 0x28, 0x2A, 0x2C, 0x2E, 0x01, 0x01, 0x01, 0x36, 0x01, 0x3A, 0x3C, 0x3E,
     0x00, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E, 0x01, 0x01, 0x01, 0x16, 0x01, 0x1A, 0x1C, 0x1E,
     0x01, 0x01, 0x01, 0x26, 0x01, 0x2A, 0x2C, 0x2E, 0x01, 0x01, 0x01, 0x36, 0x01, 0x3A, 0x3C, 0x3E};

static inline bool isZeroCrossing(void);
static inline bool noFilterZeroCrossing(void);

/*****************************************************************************
 函 数 名  : CheckZeroCrossing
 功能描述  : 过零点检测，采用择多滤波，在PWM高电平中间进行检测
 输入参数  : 无
 输出参数  : void
*****************************************************************************/
FAST_FUNC bool CheckZeroCrossing(void) {
    if (++Halless.Check_Count >= Halless.Filter_Times) {
        Halless.HallessState = 0;
        if (isZeroCrossing() == true) {
            if (Halless.FilterSwitch == true) {
                Halless.Delay_Cnt = 2;
            } else {
                Halless.Delay_Cnt = 0;
            }

            if (Halless.Check_Count < FILTER_OFF_COUNT) {
                Halless.FilterSwitch = false;
            } else if (Halless.Check_Count > FILTER_ON_COUNT) {
                Halless.FilterSwitch = true;
            }
            Halless.Zero_Flag = 1;

            Halless.Check_Count = 0;

            Halless.LastHallessState = Halless.HallessState;

            return true;
        }
    }

    return false;
}

static inline bool isZeroCrossing(void) {
    uint8_t isUp = !ADC_MASK[HoldParm.RotorDirection][HoldParm.Phase];

    if ((isUp && (ADCSample.BemfPhase > ((ADCSample.OnPhase * BEMF_MAX_COFF) >> 15))) ||
        (!isUp && (ADCSample.BemfPhase < ((ADCSample.OnPhase * BEMF_MIN_COFF) >> 15)))) {
        return false;
    }

    if (ADCSample.BemfPhase > ADCSample.OnPhaseHalf) {
        Halless.ZeroPolarity = 1;
    } else {
        Halless.ZeroPolarity = 0;
    }

    // 没有滤波
    if (Halless.FilterSwitch == false) {
        return noFilterZeroCrossing();
    }

    if (Halless.ZeroPolarity == ADC_MASK[HoldParm.RotorDirection][HoldParm.Phase]) {
        Halless.BackEMFFilter |= 0x01;
    }

    // 择多函数滤波
    Halless.BackEMFFilter = ADC_BEMF_FILTER[Halless.BackEMFFilter];

    if (Halless.BackEMFFilter & 0x01) {
        Halless.BackEMFFilter = 0;
        return true;
    }

    return false;
}

static inline bool noFilterZeroCrossing(void) {
    if (HoldParm.RotorDirection == CW) {
        switch (HoldParm.Phase) {
        case 0:
            if (Halless.ZeroPolarity == 0) {
                return true;
            }
            break;
        case 1:
            if (Halless.ZeroPolarity == 1) {
                return true;
            }
            break;
        case 2:
            if (Halless.ZeroPolarity == 0) {
                return true;
            }
            break;
        case 3:
            if (Halless.ZeroPolarity == 1) {
                return true;
            }
            break;
        case 4:
            if (Halless.ZeroPolarity == 0) {
                return true;
            }
            break;
        case 5:
            if (Halless.ZeroPolarity == 1) {
                return true;
            }
            break;
        default:
            break;
        }
    } else if (HoldParm.RotorDirection == CCW) {
        switch (HoldParm.Phase) {
        case 0:
            if (Halless.ZeroPolarity == 1) {
                return true;
            }
            break;
        case 1:
            if (Halless.ZeroPolarity == 0) {
                return true;
            }
            break;
        case 2:
            if (Halless.ZeroPolarity == 1) {
                return true;
            }
            break;
        case 3:
            if (Halless.ZeroPolarity == 0) {
                return true;
            }
            break;
        case 4:
            if (Halless.ZeroPolarity == 1) {
                return true;
            }
            break;
        case 5:
            if (Halless.ZeroPolarity == 0) {
                return true;
            }
            break;
        default:
            break;
        }
    }

    return false;
}

// HallessReset 复位
void HallessReset(void) {
    Halless.HallessState = 0;
    Halless.LastHallessState = 0;
    Halless.BackEMFFilter = 0;
    Halless.Zero_Flag = 0;
    Halless.Check_Count = 0;
    Halless.Filter_Times = 0;
    Halless.Delay_Cnt = 0;
    Halless.FilterSwitch = true;
}
