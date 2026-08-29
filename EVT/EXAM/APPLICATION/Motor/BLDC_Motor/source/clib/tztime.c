// Copyright 2019-2022 The jdh99 Authors. All rights reserved.
// Clock moudule
// Authors: jdh99 <jdh821@163.com>

#include "tztime.h"
#include <string.h>

static TZTimeGetFunc gGetTimeFunc = NULL;
static bool gIsFast = false;

// TZTimeLoad 模块载入
void TZTimeLoad(TZTimeGetFunc getTimeFunc) {
    gGetTimeFunc = getTimeFunc;
}

// TZTimeEnableFast 使能快速访问.如果芯片不支持硬件除法器,可开启快速访问
void TZTimeEnableFast(bool enable) {
    gIsFast = enable;
}

// TZTimeGet 读取时间.单位:us
uint64_t TZTimeGet(void) {
    if (gGetTimeFunc == NULL) {
        return 0;
    }
    return gGetTimeFunc();
}

// TZTimeGetMillsecond 读取时间.单位:ms
uint64_t TZTimeGetMillsecond(void) {
    if (gIsFast == false) {
        return TZTimeGet() / TZTIME_MILLISECOND;
    } else {
        return TZTimeGetMillsecondFast();
    }
}

// TZTimeGetSecond 读取时间.单位:s
uint32_t TZTimeGetSecond(void) {
    if (gIsFast == false) {
        return (uint32_t)(TZTimeGet() / TZTIME_SECOND);
    } else {
        return TZTimeGetSecondFast();
    }
}

// TZTimeGetMinute 读取时间.单位:m
uint32_t TZTimeGetMinute(void) {
    if (gIsFast == false) {
        return (uint32_t)(TZTimeGet() / TZTIME_MINUTE);
    } else {
        return TZTimeGetMinuteFast();
    }
}

// TZTimeGetHour 读取时间.单位:h
uint32_t TZTimeGetHour(void) {
    if (gIsFast == false) {
        return (uint32_t)(TZTimeGet() / TZTIME_HOUR);
    } else {
        return TZTimeGetHourFast();
    }
}

// TZTimeGetMillsecondFast 快速运算读取时间.单位:ms
uint64_t TZTimeGetMillsecondFast(void) {
    uint64_t val = TZTimeGet();
    return (val >> 10) + (val >> 15) - (val >> 17) + (val >> 21) + (val >> 24) + (val >> 26) - (val >> 29) - 
        (val >> 33) - (val >> 34) + (val >> 36) - (val >> 38) - (val >> 42) + (val >> 45) - (val >> 48) - (val >> 50) - 
        (val >> 51) + (val >> 53) - (val >> 60);
}

// TZTimeGetSecondFast 快速运算读取时间.单位:s
uint32_t TZTimeGetSecondFast(void) {
    uint64_t val = TZTimeGet();
    val = (val >> 20) + (val >> 24) - (val >> 26) + (val >> 29) - (val >> 32) - (val >> 37) - (val >> 41) + 
        (val >> 43) + (val >> 48) - (val >> 50) - (val >> 53) - (val >> 55) - (val >> 60) - (val >> 63);
    return (uint32_t)val;
}

// TZTimeGetMinuteFast 快速运算读取时间.单位:m
uint32_t TZTimeGetMinuteFast(void) {
    uint64_t val = TZTimeGet();
    val = (val >> 26) + (val >> 29) - (val >> 33) + (val >> 36) + (val >> 38) + (val >> 40) + (val >> 42) - 
        (val >> 44) + (val >> 47) - (val >> 49) + (val >> 51) - (val >> 54) + (val >> 57) + (val >> 59);
    return (uint32_t)val;
}

// TZTimeGetHourFast 快速运算读取时间.单位:h
uint32_t TZTimeGetHourFast(void) {
    uint64_t val = TZTimeGet();
    val = (val >> 32) + (val >> 34) - (val >> 36) + (val >> 39) - (val >> 41) - (val >> 44) - (val >> 46) - 
        (val >> 49) - (val >> 55) + (val >> 58) + (val >> 59) - (val >> 61);
    return (uint32_t)val;
}
