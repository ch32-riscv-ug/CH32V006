// Copyright 2019-2022 The jdh99 Authors. All rights reserved.
// Clock moudule
// Authors: jdh99 <jdh821@163.com>

#ifndef TZTIME_H
#define TZTIME_H

#include <stdint.h>
#include <stdbool.h>

// 时间单位
// 毫秒
#define TZTIME_MILLISECOND 1000ull
// 秒
#define TZTIME_SECOND 1000000ull
// 分
#define TZTIME_MINUTE 60000000ull
// 小时
#define TZTIME_HOUR 3600000000ull

// TZTimeGetFunc 读取时间的函数指针.返回值是时间,单位:us
typedef uint64_t (*TZTimeGetFunc)(void);

// TZTimeLoad 模块载入
void TZTimeLoad(TZTimeGetFunc getTimeFunc);

// TZTimeEnableFast 使能快速模式.如果芯片不支持硬件除法器,可开启快速模式
void TZTimeEnableFast(bool enable);

// TZTimeGet 读取时间.单位:us
uint64_t TZTimeGet(void);

// TZTimeGetMillsecond 读取时间.单位:ms
uint64_t TZTimeGetMillsecond(void);

// TZTimeGetSecond 读取时间.单位:s
uint32_t TZTimeGetSecond(void);

// TZTimeGetMinute 读取时间.单位:m
uint32_t TZTimeGetMinute(void);

// TZTimeGetHour 读取时间.单位:h
uint32_t TZTimeGetHour(void);

// TZTimeGetMillsecondFast 快速运算读取时间.单位:ms
uint64_t TZTimeGetMillsecondFast(void);

// TZTimeGetSecondFast 快速运算读取时间.单位:s
uint32_t TZTimeGetSecondFast(void);

// TZTimeGetMinuteFast 快速运算读取时间.单位:m
uint32_t TZTimeGetMinuteFast(void);

// TZTimeGetHourFast 快速运算读取时间.单位:h
uint32_t TZTimeGetHourFast(void);

#endif
