/********************************** (C) COPYRIGHT  *******************************
 * File Name          : tztime.h
 * Author             : WCH
 * Version            : V1.0.1
 * Date               : 2026/11/05
 * Description        : This file contains all the functions prototypes for the
 *                       firmware library.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef TZTIME_H
#define TZTIME_H

#include <stdint.h>
#include <stdbool.h>

// Time units
// Millisecond
#define TZTIME_MILLISECOND 1000ull
// Second
#define TZTIME_SECOND 1000000ull
// Minute
#define TZTIME_MINUTE 60000000ull
// Hour
#define TZTIME_HOUR 3600000000ull

// TZTimeGetFunc: function pointer that reads the time. The return value is time, unit: us
typedef uint64_t (*TZTimeGetFunc)(void);

// TZTimeLoad: module load
void TZTimeLoad(TZTimeGetFunc getTimeFunc);

// TZTimeEnableFast: enable fast mode. If the chip has no hardware divider, fast mode can be enabled
void TZTimeEnableFast(bool enable);

// TZTimeGet: read the time, unit: us
uint64_t TZTimeGet(void);

// TZTimeGetMillsecond: read the time, unit: ms
uint64_t TZTimeGetMillsecond(void);

// TZTimeGetSecond: read the time, unit: s
uint32_t TZTimeGetSecond(void);

// TZTimeGetMinute: read the time, unit: m
uint32_t TZTimeGetMinute(void);

// TZTimeGetHour: read the time, unit: h
uint32_t TZTimeGetHour(void);

// TZTimeGetMillsecondFast: read the time with fast arithmetic, unit: ms
uint64_t TZTimeGetMillsecondFast(void);

// TZTimeGetSecondFast: read the time with fast arithmetic, unit: s
uint32_t TZTimeGetSecondFast(void);

// TZTimeGetMinuteFast: read the time with fast arithmetic, unit: m
uint32_t TZTimeGetMinuteFast(void);

// TZTimeGetHourFast: read the time with fast arithmetic, unit: h
uint32_t TZTimeGetHourFast(void);

#endif
