/********************************** (C) COPYRIGHT  *******************************
 * File Name          : tztype.h
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

#ifndef TZTYPE_H
#define TZTYPE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// Unused parameter declaration, avoids compiler warnings
#define TZ_UNUSED(v) (void)v

// Interrupt priority
#define TZ_IRQ_PRIORITY_HIGHEST 0
#define TZ_IRQ_PRIORITY_HIGH 1
#define TZ_IRQ_PRIORITY_MIDDLE 2
#define TZ_IRQ_PRIORITY_LOW 3
#define TZ_IRQ_PRIORITY_LOWEST 4

#pragma pack(1)

// Buffer data structure
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

// TZBufferDynamic dynamic buffer
typedef struct {
    int len;
    uint8_t buf[];
} TZBufferDynamic;

#pragma pack()

// Return values of frame handling
typedef enum {
    // Pass through
    TZ_PASS,
    // Block
    TZ_STOP
} TZDealFrameReturn;

// TZIsAllowSendFunc: whether sending is allowed function type
typedef bool (*TZIsAllowSendFunc)(void);

// TZEmptyFunc: empty function type
typedef void (*TZEmptyFunc)(void);

// TZDataFunc: data function type
typedef void (*TZDataFunc)(uint8_t* bytes, int size);

// TZNetDataFunc: network data function type
typedef void (*TZNetDataFunc)(uint8_t* bytes, int size, uint32_t ip, uint16_t port);

// TZPipeDataFunc: pipe data function type
typedef void (*TZPipeDataFunc)(uint8_t* bytes, int size, uint32_t ip, uint16_t port, int pipe);

// TZDataByIAFunc: send data by IA address function type
typedef void (*TZDataByIAFunc)(uint8_t* bytes, int size, uint32_t ia);

// TZWriteIO: set the pin level
typedef void (*TZWriteIO)(bool level);

// TZReadIO: read the pin level
typedef bool (*TZReadIO)(void);

#endif
