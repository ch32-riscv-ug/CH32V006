/********************************** (C) COPYRIGHT  *******************************
 * File Name          : param.c
 * Author             : WCH
 * Version            : V1.0.1
 * Date               : 2026/11/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "framework.h"

volatile MotorState_T mcState;
volatile ErrorState_T ErrorCode;
volatile HoldControlPara_T HoldParm;
volatile ADCSamplePara_T ADCSample;

// ParamLoad: parameter loading
void ParamLoad(void) {
    PWMSetAllOff();

    ProtectClearError();

    mcState = mcIdle;

    ADCSample.Voltage = 0;
    ADCSample.PhaseVoltage[0] = 0;
    ADCSample.PhaseVoltage[1] = 0;
    ADCSample.PhaseVoltage[2] = 0;
    ADCSample.NeutralPoint = 0;

    HoldParm.RotorDirection = CW;
    HoldParm.Phase = 0;
    HoldParm.ControlLoopCnt = 0;
    ParamReset();
}

// ParamReset: parameter reset
void ParamReset(void) {
    HoldParm.SpeedTime_Cnt = 0;
    HoldParm.SpeedTime_Sum = 0;
    HoldParm.Speed = 0;
    HoldParm.SpeedFilter = 0;
    HoldParm.ZeroCrossTime = 0;
    HoldParm.ZeroCrossTimeSum = 0;
    HoldParm.LastZeroCrossTimeSum = 0;
    HoldParm.LastZeroCrossTime = 0;
    HoldParm.PhaseDelayCompensation = PHASE_DELAY_COMPENSATION_CNT;
    HoldParm.IsSyncChargingMode = (SYNC_CHARGING_SWITCH == 1);
    HoldParm.PhaseIsChange = false;
    HoldParm.DesyncCheck = false;
    HoldParm.ControlLoopCnt = 0;
    HoldParm.PWMDutyCycle = 0;
    HoldParm.Count = 0;
    if (HoldParm.SensorType == CMP_SENSOR) {
        HoldParm.IsCmpMode = true;
    } else {
        HoldParm.IsCmpMode = false;
    }
}
