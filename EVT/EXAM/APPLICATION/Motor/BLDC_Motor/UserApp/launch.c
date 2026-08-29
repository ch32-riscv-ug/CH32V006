/********************************** (C) COPYRIGHT  *******************************
 * File Name          : launch.c
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
#include "control.h"
#include "upload.h"
#include "drivers.h"
#include "isr.h"
#include "key.h"
#include "protect.h"
#include "pwm.h"
#include "speed.h"
#include "startup.h"
#include "warmboot.h"

static void keyEvent(int key, KeyEvent event);
static int test(void);

static uint8_t keyBuffer[32] = {0};

static bool testFlag = false;
static uint16_t testCnt = 0;
static uint16_t failCnt = 0;

// LaunchLoad 启动器载入
void LaunchLoad(void) {
    KeyLoad(keyBuffer, sizeof(keyBuffer));
    KeyRegister(0, DrvKeyGet);
    KeyRegisterObserver(keyEvent);
}

void LaunchRun(void) {
    KeyRun();
    SpeedRun();
    UploadSend(ADCSample.Voltage, ADCSample.OnPhase, ADCSample.BemfPhase, TIM1->CH1CVR);
    // test();
}

static void keyEvent(int key, KeyEvent event) {
    if (KEY_EVENT_SHORT_PUSH == event) {
        if (mcState != mcIdle) {
            MotorStop();
            testFlag = false;
            failCnt = 0;
            testCnt = 0;
        } else if (mcState == mcIdle) {
            SpeedSet(10000);
            MotorStart();
            testFlag = true;
            failCnt = 0;
            testCnt = 0;
        }
    }
}

static int test(void) {
    static struct pt pt = {0};
    static uint64_t time = 0;
    static uint16_t duty = PWM_MIN_DUTY;

    PT_BEGIN(&pt);

    PT_WAIT_UNTIL(&pt, testFlag == true);
    ErrorCode.value = 0;

    MotorStart();
    duty = PWM_MIN_DUTY;
    MotorSetDuty(duty);

    for (;;) {
        duty += PWM_DUTYCYCLE_01;
        MotorSetDuty(duty);
        if (duty > PWM_MAX_DUTY) {
            duty = PWM_MAX_DUTY;
            break;
        }
        if (testFlag == false || ErrorCode.value != 0) {
            break;
        }

        time = TZTimeGetMillsecond();
        PT_WAIT_UNTIL(&pt, TZTimeGetMillsecond() - time > 100);
    }

    PT_WAIT_UNTIL(&pt, testFlag == false || ErrorCode.value != 0);
    // if (ErrorCode.bit.Stall == 1) {
    //     MotorStop();
    //     testFlag = false;
    //     PT_EXIT (&pt);
    // }

    for (;;) {
        duty -= PWM_DUTYCYCLE_01;
        MotorSetDuty(duty);
        if (duty < PWM_MIN_DUTY) {
            duty = PWM_MIN_DUTY;
            break;
        }
        time = TZTimeGetMillsecond();
        PT_WAIT_UNTIL(&pt, TZTimeGetMillsecond() - time > 50);
        PT_YIELD(&pt);
    }

    testFlag = false;

    time = TZTimeGetMillsecond();
    PT_WAIT_UNTIL(&pt, ((TIM1->CH1CVR <= PWM_DUTYCYCLE_10) && (HoldParm.SpeedFilter <= 3276)) || TZTimeGetMillsecond() - time > 5000 || testFlag == true);
    if (testFlag == true) {
        PT_EXIT(&pt);
    }

    MotorStop();

    PT_END(&pt);
}
