/********************************** (C) COPYRIGHT  *******************************
 * File Name          : speed.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2026/08/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "speed.h"
#include "isr.h"
#include "pwm.h"

static uint32_t gSpeedTimeSum = 0;
static uint32_t gRpmFiltered = 0; // IIR滤波后的RPM

#define IIR_ALPHA 0.1f

static PIParam gPIParam;

static inline int speedloop(void);

// SpeedInit 初始化
void SpeedInit(void) {
    PISetParam(&gPIParam, SPEED_KP, SPEED_KI, SPEED_OUT_MAX, SPEED_OUT_MIN);
}

// SpeedRun 运行
void SpeedRun(void) {
    if (gSpeedTimeSum != 0) {
        uint32_t rawRpm = (SYSTEM_CLOCK_FREQ / (SPEED_MAX / 60.0f) * _IQ15(1.0f)) / (gSpeedTimeSum);
        gSpeedTimeSum = 0;

        if (gRpmFiltered == 0) {
            gRpmFiltered = rawRpm; // 首次直接赋值
        } else {
            gRpmFiltered += _IQ15mpy(_IQ15(IIR_ALPHA), rawRpm - gRpmFiltered);
        }
        HoldParm.SpeedFilter = gRpmFiltered;
        HoldParm.Speed = rawRpm;
    }

    if (mcState != mcRun){
        SpeedReset();
        return;
    }

    speedloop();
}

static inline int speedloop(void) {
    static struct pt pt = {0};
    static uint64_t time = 0;

    PT_BEGIN(&pt);

    PT_WAIT_UNTIL(&pt, mcState == mcRun);

    MotorSetDuty(_IQ15mpy(PICalc(&gPIParam, HoldParm.SetSpeed, HoldParm.SpeedFilter), PWM_FRE_SETATA));

    time = TZTimeGet();
    PT_WAIT_UNTIL(&pt, TZTimeGet() - time > 1000);

    PT_END(&pt);
}

// SpeedSetTime 设置时间
void SpeedSetTime(uint32_t time) {
    if (gSpeedTimeSum != 0) {
        return;
    }
    gSpeedTimeSum = time;
}

void SpeedSetParam(float kp, float ki, float outMax, float outMin) {
    PISetParam(&gPIParam, kp, ki, outMax, outMin);
}

// SpeedSet 设置转速
void SpeedSet(uint32_t eRPM) {
    if (eRPM < SPEED_MIN) {
        eRPM = SPEED_MIN;
    } else if (eRPM > SPEED_MAX) {
        eRPM = SPEED_MAX;
    }
    HoldParm.SetSpeed = _IQ15(eRPM / (float)SPEED_MAX);
}

// SpeedGet 获取转速 
uint32_t SpeedGet(void) {
    return _IQ15toF(HoldParm.SetSpeed) * SPEED_MAX;
}

// SpeedReset 重置
void SpeedReset(void) {
    gSpeedTimeSum = 0;
    gRpmFiltered = 0;
    HoldParm.SpeedFilter = 0;
    HoldParm.Speed = 0;
    HoldParm.PWMDutyCycle = 0;
    HoldParm.SpeedTime_Sum = 0;
    HoldParm.SpeedTime_Cnt = 0;
    HoldParm.SpeedLoopCnt = 0;
    PIReset(&gPIParam);
}
