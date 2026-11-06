/********************************** (C) COPYRIGHT  *******************************
 * File Name          : tztime.c
 * Author             : WCH
 * Version            : V1.0.1
 * Date               : 2026/11/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#include "tztime.h"
#include <string.h>

static TZTimeGetFunc gGetTimeFunc = NULL;
static bool gIsFast = false;

// TZTimeLoad: module load
void TZTimeLoad(TZTimeGetFunc getTimeFunc) {
    gGetTimeFunc = getTimeFunc;
}

// TZTimeEnableFast: enable fast access. If the chip has no hardware divider, fast access can be enabled
void TZTimeEnableFast(bool enable) {
    gIsFast = enable;
}

// TZTimeGet: read the time, unit: us
uint64_t TZTimeGet(void) {
    if (gGetTimeFunc == NULL) {
        return 0;
    }
    return gGetTimeFunc();
}

// TZTimeGetMillsecond: read the time, unit: ms
uint64_t TZTimeGetMillsecond(void) {
    if (gIsFast == false) {
        return TZTimeGet() / TZTIME_MILLISECOND;
    } else {
        return TZTimeGetMillsecondFast();
    }
}

// TZTimeGetSecond: read the time, unit: s
uint32_t TZTimeGetSecond(void) {
    if (gIsFast == false) {
        return (uint32_t)(TZTimeGet() / TZTIME_SECOND);
    } else {
        return TZTimeGetSecondFast();
    }
}

// TZTimeGetMinute: read the time, unit: m
uint32_t TZTimeGetMinute(void) {
    if (gIsFast == false) {
        return (uint32_t)(TZTimeGet() / TZTIME_MINUTE);
    } else {
        return TZTimeGetMinuteFast();
    }
}

// TZTimeGetHour: read the time, unit: h
uint32_t TZTimeGetHour(void) {
    if (gIsFast == false) {
        return (uint32_t)(TZTimeGet() / TZTIME_HOUR);
    } else {
        return TZTimeGetHourFast();
    }
}

// TZTimeGetMillsecondFast: read the time with fast arithmetic, unit: ms
uint64_t TZTimeGetMillsecondFast(void) {
    uint64_t val = TZTimeGet();
    return (val >> 10) + (val >> 15) - (val >> 17) + (val >> 21) + (val >> 24) + (val >> 26) - (val >> 29) - 
        (val >> 33) - (val >> 34) + (val >> 36) - (val >> 38) - (val >> 42) + (val >> 45) - (val >> 48) - (val >> 50) - 
        (val >> 51) + (val >> 53) - (val >> 60);
}

// TZTimeGetSecondFast: read the time with fast arithmetic, unit: s
uint32_t TZTimeGetSecondFast(void) {
    uint64_t val = TZTimeGet();
    val = (val >> 20) + (val >> 24) - (val >> 26) + (val >> 29) - (val >> 32) - (val >> 37) - (val >> 41) + 
        (val >> 43) + (val >> 48) - (val >> 50) - (val >> 53) - (val >> 55) - (val >> 60) - (val >> 63);
    return (uint32_t)val;
}

// TZTimeGetMinuteFast: read the time with fast arithmetic, unit: m
uint32_t TZTimeGetMinuteFast(void) {
    uint64_t val = TZTimeGet();
    val = (val >> 26) + (val >> 29) - (val >> 33) + (val >> 36) + (val >> 38) + (val >> 40) + (val >> 42) - 
        (val >> 44) + (val >> 47) - (val >> 49) + (val >> 51) - (val >> 54) + (val >> 57) + (val >> 59);
    return (uint32_t)val;
}

// TZTimeGetHourFast: read the time with fast arithmetic, unit: h
uint32_t TZTimeGetHourFast(void) {
    uint64_t val = TZTimeGet();
    val = (val >> 32) + (val >> 34) - (val >> 36) + (val >> 39) - (val >> 41) - (val >> 44) - (val >> 46) - 
        (val >> 49) - (val >> 55) + (val >> 58) + (val >> 59) - (val >> 61);
    return (uint32_t)val;
}
