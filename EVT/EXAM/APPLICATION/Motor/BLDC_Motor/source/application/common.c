/********************************** (C) COPYRIGHT  *******************************
 * File Name          : common.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2026/08/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "common.h"
#include "halless.h"
#include "pwm.h"

volatile MotorState_T mcState;
volatile ErrorState_T ErrorCode;
volatile HoldControlPara_T HoldParm;
volatile ADCSamplePara_T ADCSample;
volatile PWMCatchPara_T CatchParm;
volatile SensorPara_T Halless;

/*****************************************************************************
 函 数 名  : PowerupParaInit
 功能描述  : 上电参数初始化
 输入参数  : 无
 输出参数  : void
*****************************************************************************/
void PowerupParaInit(void) {
    PWMSetAllOff();
    ErrorCode.value = 0;
    HoldParm.RotorDirection = CW;
    mcState = mcIdle;
}

/*****************************************************************************
 函 数 名  : Common_Init
 功能描述  : 全局变量初始化
 输入参数  : 无
 输出参数  : void
*****************************************************************************/
void Common_Init(void) {
    HoldParm.MainDetectCnt = 0;
    HoldParm.SpeedLoopCnt = 0;
    HoldParm.PWMDutyCycle = 0;
    HoldParm.SpeedTime = 0;
    HoldParm.DragCnt = DRAG_CNT_MAX;
    HoldParm.Speed = 0;
    HoldParm.SpeedFilter = 0;
    HoldParm.SpeedTime_Cnt = 0;
    HoldParm.SpeedTime_Sum = 0;
    HoldParm.SpeedTimeTemp = 0;
    HoldParm.ZeroCrossTime = 0;
    HoldParm.LastZeroCrossTime = 0;
    HoldParm.Phase = 0;
    HoldParm.PhaseIsChange = false;
    HoldParm.FirstZeroCrossTimestamp = 0;
    HoldParm.IsCmpMode = false;

    CatchParm.Period = 0;
    CatchParm.Duty = 0;
    CatchParm.Flag_Cap_Valid = 0;
    CatchParm.PWMCnt = 0;
    CatchParm.DutyCycleUse = 0;
    ErrorCode.value = 0;

    ADCSample.Voltage = 0;
    ADCSample.UBemf = 0;
    ADCSample.VBemf = 0;
    ADCSample.WBemf = 0;
    ADCSample.NeutralPoint = 0;

    HallessReset();
}
