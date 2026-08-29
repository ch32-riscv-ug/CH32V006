/********************************** (C) COPYRIGHT  *******************************
 * File Name          : pi_regulator.h
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
#ifndef PI_REGULATOR_H
#define PI_REGULATOR_H

#include "def.h"

typedef enum {
    PI_MODE_PARALLEL = 0,
    PI_MODE_SERIES,
} PIMode;

typedef struct {
    PIMode Mode;
    int32_t KP;
    int32_t KI;
    int32_t UP;
    int32_t UI;
    int32_t UPShift;
    int32_t UIShift;
    int32_t OutMax;
    int32_t OutMin;
    int32_t Out;
    int32_t Err;
} PIParam;

// PISetParam 设置PI控制器参数
void PISetParam(PIParam *param, float kp, float ki, float outMax, float outMin);

// PIReset PI控制器复位
void PIReset(PIParam *param);

// PICalc PI控制器计算
// 只输入误差时, ref = err, fbk = 0
int32_t PICalc(PIParam *param, int32_t ref, int32_t fbk);

#endif
