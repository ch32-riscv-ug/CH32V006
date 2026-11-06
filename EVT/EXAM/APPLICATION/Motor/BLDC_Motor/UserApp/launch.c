/********************************** (C) COPYRIGHT  *******************************
 * File Name          : launch.c
 * Author             : WCH
 * Version            : V1.0.1
 * Date               : 2026/11/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "launch.h"
#include "control.h"
#include "key.h"
#include "speed.h"
#include "upload.h"

static void keyEvent(int key, KeyEvent event);

// LaunchLoad: launcher load initialization
void LaunchLoad(void) {
    KeyRegister(0, DrvKeyGet);
    KeyRegisterObserver(keyEvent);
    HoldParm.SensorType = ADC_SENSOR;
    HoldParm.RotorDirection = CW;
}

void LaunchRun(void) {
    AdcSampleLoop();
    // Debug: bus voltage / actual speed (eRPM) / current / error code
    UploadSend(ADCSample.Voltage,
               _IQ15toF(HoldParm.SpeedFilter) * SPEED_MAX,
               ADCSample.Current,
               ErrorCode.value);
}

static void keyEvent(int key, KeyEvent event) {
    if (KEY_EVENT_SHORT_PUSH == event) {
        if (mcState != mcIdle) {
            MotorStop(BRAKE_TIME);
        } else if (mcState == mcIdle) {
            SpeedSet(20000);
            MotorStart();
        }
    }
}
