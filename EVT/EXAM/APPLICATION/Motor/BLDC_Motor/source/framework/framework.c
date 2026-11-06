/********************************** (C) COPYRIGHT  *******************************
 * File Name          : framework.c
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
#include "control.h"

// Software protection (over-voltage/under-voltage/soft over-current) switch, defined by each project's config.h.
// When undefined the original behavior is kept (not running), so other projects sharing source/ are not falsely protected due to uncalibrated thresholds
#ifndef PROTECT_ENABLE
#define PROTECT_ENABLE 0
#endif

// Independent watchdog switch, defined by each project's config.h. When undefined it is not enabled,
// so old projects sharing source/ are not forcibly reset without knowing it
#ifndef IWDG_ENABLE
#define IWDG_ENABLE 0
#endif

static uint8_t gFrameworkBuf[128];

// FrameworkLoad: load the framework
void FrameworkLoad(void) {
    DriversInit();
    ParamLoad();
    KeyLoad(gFrameworkBuf, sizeof(gFrameworkBuf));
    ProtectInit();
    HallessLoad(STARTUP_ZERO_THRESHOLD, ADC_SUPPORT_HIGH_SPEED);
    MotorAdcSamplingReset();
}

// FrameworkRun: run the framework
void FrameworkRun(void) {
    KeyRun();

#if PROTECT_ENABLE == 1
    ProtectRun();
#endif

#if IWDG_ENABLE == 1
    // Main-loop watchdog feed: reset the whole chip if the main loop hangs longer than IWDG_TIMEOUT_MS
    DrvWdgFeed();
#endif
}
