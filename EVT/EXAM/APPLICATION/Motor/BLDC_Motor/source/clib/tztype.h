// Copyright 2021-2021 The jdh99 Authors. All rights reserved.
// 常用基础数据类型
// Authors: jdh99 <jdh821@163.com>

#ifndef TZTYPE_H
#define TZTYPE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// 未使用命令声明.可避免编译器警告
#define TZ_UNUSED(v) (void)v

// 中断优先级
#define TZ_IRQ_PRIORITY_HIGHEST 0
#define TZ_IRQ_PRIORITY_HIGH 1
#define TZ_IRQ_PRIORITY_MIDDLE 2
#define TZ_IRQ_PRIORITY_LOW 3
#define TZ_IRQ_PRIORITY_LOWEST 4

#pragma pack(1)

// 缓冲区数据结构
#define TZ_BUFFER_TINY_LEN 32
typedef struct {
    int len;
    uint8_t buf[TZ_BUFFER_TINY_LEN];
} TZBufferTiny;

#define TZ_BUFFER_SMALL_LEN 64
typedef struct {
    int len;
    uint8_t buf[TZ_BUFFER_SMALL_LEN];
} TZBufferSmall;

#define TZ_BUFFER_LEN 128
typedef struct {
    int len;
    uint8_t buf[TZ_BUFFER_LEN];
} TZBuffer;

#define TZ_BUFFER_BIG_LEN 256
typedef struct {
    int len;
    uint8_t buf[TZ_BUFFER_BIG_LEN];
} TZBufferBig;

#define TZ_BUFFER_LARGE_LEN 512
typedef struct {
    int len;
    uint8_t buf[TZ_BUFFER_LARGE_LEN];
} TZBufferLarge;

// TZBufferDynamic 动态缓存
typedef struct {
    int len;
    uint8_t buf[];
} TZBufferDynamic;

#pragma pack()

// 处理帧的返回值
typedef enum {
    // 放行
    TZ_PASS,
    // 拦截
    TZ_STOP
} TZDealFrameReturn;

// TZIsAllowSendFunc 是否允许发送函数类型
typedef bool (*TZIsAllowSendFunc)(void);

// TZEmptyFunc 空函数类型
typedef void (*TZEmptyFunc)(void);

// TZDataFunc 数据函数类型
typedef void (*TZDataFunc)(uint8_t* bytes, int size);

// TZNetDataFunc 网络数据函数类型
typedef void (*TZNetDataFunc)(uint8_t* bytes, int size, uint32_t ip, uint16_t port);

// TZPipeDataFunc 管道数据函数类型
typedef void (*TZPipeDataFunc)(uint8_t* bytes, int size, uint32_t ip, uint16_t port, int pipe);

// TZDataByIAFunc 根据ia地址发送数据函数类型
typedef void (*TZDataByIAFunc)(uint8_t* bytes, int size, uint32_t ia);

// TZWriteIO 设置引脚电平
typedef void (*TZWriteIO)(bool level);

// TZReadIO 读取引脚电平
typedef bool (*TZReadIO)(void);

#endif
