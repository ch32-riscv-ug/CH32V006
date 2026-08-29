/********************************** (C) COPYRIGHT  *******************************
 * File Name          : pi_regulator.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2026/08/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "pi_regulator.h"

#define _IQnmpy(a, b, c) ((a * b) >> c)

static void series(PIParam *param, int32_t ref, int32_t fbk);
static void parallel(PIParam *param, int32_t ref, int32_t fbk);

// PISetParam 设置PI控制器参数
void PISetParam(PIParam *param, float kp, float ki, float outMax, float outMin) {
    param->UPShift = 15;
    param->UIShift = 15;
    param->KP = _IQ15(kp);
    param->KI = _IQ15(ki);
    param->OutMax = _IQ15(outMax);
    param->OutMin = _IQ15(outMin);
    while (param->KP > _IQ15(1.0f)) {
        param->KP >>= 1;
        param->UPShift--;
    }
    while (param->KI > _IQ15(1.0f)) {
        param->KI >>= 1;
        param->UIShift--;
    }

    PIReset(param);
}

/**
 * @brief PI控制器复位
 * @param param PI结构体
 * @return None
 * @author GuMingYang
 * @date 2021-06-03
 * @version 1.0 
 */
void PIReset(PIParam *param) {
    param->Err = 0;
    param->UP = 0;
    param->UI = 0;
    param->Out = 0;
}

static void series(PIParam *param, int32_t ref, int32_t fbk) {
    param->Err = ref - fbk;
    param->UP = _IQnmpy(param->KP, param->Err, param->UPShift);
    param->UI += _IQnmpy(param->KI, param->UP, param->UIShift);
    param->UI = _IQsat(param->UI, param->OutMax, param->OutMin);
    param->Out = _IQsat(param->UP + param->UI, param->OutMax, param->OutMin);
}

static void parallel(PIParam *param, int32_t ref, int32_t fbk) {
    param->Err = ref - fbk;
    param->UP = _IQnmpy(param->KP, param->Err, param->UPShift);
    param->UI += _IQnmpy(param->KI, param->Err, param->UIShift);
    param->UI = _IQsat(param->UI, param->OutMax, param->OutMin);
    param->Out = _IQsat(param->UP + param->UI, param->OutMax, param->OutMin);
}

// PICalc PI控制器计算
// 只输入误差时, ref = err, fbk = 0
int32_t PICalc(PIParam *param, int32_t ref, int32_t fbk) {
    if (param->Mode == PI_MODE_PARALLEL) {
        parallel(param, ref, fbk);
    } else if (param->Mode == PI_MODE_SERIES) {
        series(param, ref, fbk);
    }
    return param->Out;
}
