/********************************** (C) COPYRIGHT  *******************************
 * File Name          : upload.c
 * Author             : WCH
 * Version            : V1.0.1
 * Date               : 2026/11/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "upload.h"

// The SDI debug data register address differs between cores; select it by the chip model macros defined in the chip header:
//   V00x / M007 : 0xE00000F4 (QingKe V4A/V2 series debug module)
//   V20x / V30x : 0xE0000380
//   M030 and other new cores : 0xE0000340 (official value in EVT debug.c)
// Accessing a wrong address triggers a load/store fault -> HardFault -> reset,
// which shows up as "the program runs for a moment and is kicked back to reset". New cores use the M030 address by default;
// if needed, define SDI_DATA0_ADDR/SDI_DATA1_ADDR in def.h/config.h to override it.
#ifndef SDI_DATA0_ADDR
    #if defined(CH32V002) || defined(CH32V004) || defined(CH32V005) || \
        defined(CH32V006) || defined(CH32V007_M007)
        #define SDI_DATA0_ADDR 0xE00000F4
        #define SDI_DATA1_ADDR 0xE00000F8
    #elif defined(CH32V20x_D6) || defined(CH32V20x_D8) || defined(CH32V20x_D8W) || \
          defined(CH32V30x_D8) || defined(CH32V30x_D8C)
        #define SDI_DATA0_ADDR 0xE0000380
        #define SDI_DATA1_ADDR 0xE0000384
    #else
        #define SDI_DATA0_ADDR 0xE0000340
        #define SDI_DATA1_ADDR 0xE0000344
    #endif
#endif

#define DEBUG_DATA0_ADDRESS ((volatile uint32_t *)SDI_DATA0_ADDR)
#define DEBUG_DATA1_ADDRESS ((volatile uint32_t *)SDI_DATA1_ADDR)

// ==================== Legacy protocol: UploadSend ====================
// Frame format: 4 floats + frame tail {0x00, 0x00, 0x80, 0x7F}

typedef struct {
    float data[4];
    uint8_t tail[4];
} tData;

static tData debugData = {.tail = {0x00, 0x00, 0x80, 0x7f}};

// ==================== Virtual oscilloscope protocol: VScopeSend ====================
// Data frame format (no checksum, integrity relies on the heartbeat):
//   [0xAA][0x55][LEN][HeartBeat][sample data...]
//   LEN = 4 + N(channel count) * channel bytes * M(samples per frame), max 64 bytes
//   HeartBeat increments by 1 for every frame sent successfully (wraps 0~255); the first frame after upload starts must be 1
//   Fixed-point mode: int16_t little-endian 2 bytes per channel; float mode: float little-endian 4 bytes per channel
//   Within one sample point the data is stored in channel order 0,1,...,N-1 and all channels share the same horizontal coordinate

#ifndef UPLOAD_CHANNEL_NUM
#define UPLOAD_CHANNEL_NUM 4 // Number of upload channels (1~4, VScopeSend provides 4 channels of data)
#endif

#ifndef UPLOAD_DATA_INT16
#define UPLOAD_DATA_INT16 0 // 0=float mode, 1=fixed-point int16 mode
#endif

#if UPLOAD_CHANNEL_NUM < 1 || UPLOAD_CHANNEL_NUM > 4
#error "UPLOAD_CHANNEL_NUM must be in the range 1~4 (VScopeSend only provides 4 channels of data)"
#endif

#define FRAME_HEAD0 0xAAu
#define FRAME_HEAD1 0x55u
#define FRAME_MAX_LEN 64u // Protocol maximum frame length (bytes)

#if UPLOAD_DATA_INT16 == 1
#define UPLOAD_CH_SIZE 2u // Fixed-point int16, 2 bytes per channel
#else
#define UPLOAD_CH_SIZE 4u // Float, 4 bytes per channel
#endif

// Samples per frame M = maximum 60 bytes of data area / bytes per sample point, consistent with the protocol frame parameter table
#define UPLOAD_POINT_SIZE (UPLOAD_CHANNEL_NUM * UPLOAD_CH_SIZE)
#define UPLOAD_POINTS_PER_FRAME ((FRAME_MAX_LEN - 4u) / UPLOAD_POINT_SIZE)
#define UPLOAD_FRAME_LEN (4u + UPLOAD_POINT_SIZE * UPLOAD_POINTS_PER_FRAME)

static uint8_t frameBuf[FRAME_MAX_LEN]; // Data frame buffer
static uint8_t pointCount = 0;          // Number of sample points currently buffered
static uint8_t heartBeat = 0;           // Heartbeat counter (1 for the first frame)

static uint8_t sdiBuffer[FRAME_MAX_LEN];
static uint8_t sdiSize = 0;
static uint8_t sdiOffset = 0;

void sdiOut(uint8_t *buf, int size);

void UploadSend(float data1, float data2, float data3, float data4) {
    debugData.data[0] = data1;
    debugData.data[1] = data2;
    debugData.data[2] = data3;
    debugData.data[3] = data4;
#if SDI_DEBUG_ENABLE == 1
    sdiOut((uint8_t *)&debugData, sizeof(debugData));
#elif UART_DEBUG_ENABLE == 1
    DrvUartSendBytes((uint8_t *)&debugData, sizeof(debugData));
#else
#ifdef USB_DEBUG
    Deal_sendMonitorData((uint8_t *)&debugData, sizeof(debugData));
#endif
#endif
}

// vscopePutSample: buffer one sample point (N channels) into the frame data area
static void vscopePutSample(float data1, float data2, float data3, float data4) {
    const float ch[4] = {data1, data2, data3, data4};
    uint8_t *p = &frameBuf[4 + pointCount * UPLOAD_POINT_SIZE];
    for (uint8_t i = 0; i < UPLOAD_CHANNEL_NUM; i++) {
#if UPLOAD_DATA_INT16 == 1
        // Convert float to int16 with saturation, stored little-endian
        float v = ch[i];
        if (v > 32767.0f) {
            v = 32767.0f;
        } else if (v < -32768.0f) {
            v = -32768.0f;
        }
        int16_t s = (int16_t)v;
        p[0] = (uint8_t)((uint16_t)s & 0xFFu);
        p[1] = (uint8_t)((uint16_t)s >> 8);
        p += 2;
#else
        memcpy(p, &ch[i], sizeof(float)); // Little-endian MCU, store the raw memory bit pattern directly
        p += 4;
#endif
    }
    pointCount++;
}

// vscopeFlushFrame: fill the frame header and send one data frame
static void vscopeFlushFrame(void) {
    frameBuf[0] = FRAME_HEAD0;
    frameBuf[1] = FRAME_HEAD1;
    frameBuf[2] = UPLOAD_FRAME_LEN;
    frameBuf[3] = ++heartBeat;
#if SDI_DEBUG_ENABLE == 1
    sdiOut(frameBuf, UPLOAD_FRAME_LEN);
#endif
#if UART_DEBUG_ENABLE == 1
    DrvUartSendBytes(frameBuf, UPLOAD_FRAME_LEN);
#endif
#ifdef USB_DEBUG
    Deal_sendMonitorData(frameBuf, UPLOAD_FRAME_LEN);
#endif
}

// VScopeSend: virtual oscilloscope data upload (one sample point per call, frame is assembled and sent once M points are buffered)
void VScopeSend(float data1, float data2, float data3, float data4) {
#if SDI_DEBUG_ENABLE == 1
    sdiOut(NULL, 0); // Keep pushing the remaining chunks of the previous frame
#endif
    vscopePutSample(data1, data2, data3, data4);
    if (pointCount >= UPLOAD_POINTS_PER_FRAME) {
        pointCount = 0;
        vscopeFlushFrame();
    }
}

void sdiOut(uint8_t *buf, int size) {
    if (sdiSize == 0) {
        if (buf == NULL || size <= 0) {
            return;
        }
        sdiSize = (size > (int)sizeof(sdiBuffer)) ? (uint8_t)sizeof(sdiBuffer) : (uint8_t)size;
        sdiOffset = 0;
        for (uint8_t i = 0; i < sdiSize; i++) {
            sdiBuffer[i] = buf[i];
        }
    }

    if (*(DEBUG_DATA0_ADDRESS) != 0u) {
        return;
    }

    /**
     * data0  data1 8 byte
     * data0 The storage length of the lowest byte, with a maximum of 7 bytes.
     */
    uint8_t remaining = sdiSize - sdiOffset;
    uint8_t chunkSize = (remaining > 7) ? 7 : remaining;
    uint32_t data0 = chunkSize;
    uint32_t data1 = 0;
    for (uint8_t i = 0; i < chunkSize; i++) {
        if (i < 3) {
            data0 |= (uint32_t)sdiBuffer[sdiOffset + i] << ((i + 1) * 8);
        } else {
            data1 |= (uint32_t)sdiBuffer[sdiOffset + i] << ((i - 3) * 8);
        }
    }

    *(DEBUG_DATA1_ADDRESS) = data1;
    *(DEBUG_DATA0_ADDRESS) = data0;
    sdiOffset += chunkSize;
    if (sdiOffset >= sdiSize) {
        sdiSize = 0;
        sdiOffset = 0;
    }
}
