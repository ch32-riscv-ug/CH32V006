/********************************** (C) COPYRIGHT  *******************************
 * File Name          : main.c
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
#include "isr.h"
#include "key.h"
#include "protect.h"
#include "pwm.h"
#include "speed.h"
#include "startup.h"
#include "warmboot.h"
#include "launch.h"

int main() {
    DriversInit();
    Common_Init();
    PowerupParaInit();
    TZTimeLoad(SysTimeGet);
    TZTimeEnableFast(true);
    ProtectInit();
    SpeedInit();
    StartupLoad(STARTUP_ZERO_THRESHOLD, STARTUP_ZCP_CNT, ADC_SUPPORT_HIGH_SPEED);
    WarmBootInit(WARMBOOT_VOLTAGE_THRESHOLD);
    LaunchLoad();
    while (1) {
        LaunchRun();
        ProtectRun();
    }
}
