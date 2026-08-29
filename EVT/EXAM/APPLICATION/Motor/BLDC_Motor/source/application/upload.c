/********************************** (C) COPYRIGHT  *******************************
 * File Name          : upload.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2026/08/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "upload.h"

#define DEBUG_DATA0_ADDRESS ((volatile uint32_t *)0xE00000F4)
#define DEBUG_DATA1_ADDRESS ((volatile uint32_t *)0xE00000F8)

typedef struct {
    float data[4];
    uint8_t tail[4];
} tData;

static tData debugData = {.tail = {0x00, 0x00, 0x80, 0x7f}};

void sdiOut(uint8_t *buf, int size);

void UploadSend(float data1, float data2, float data3, float data4) {
    debugData.data[0] = data1;
    debugData.data[1] = data2;
    debugData.data[2] = data3;
    debugData.data[3] = data4;
#if SDI_DEBUG_ENABLE == 1

    // GPIO_WriteBit(GPIOC, GPIO_Pin_0, Bit_SET);
    sdiOut((uint8_t *)&debugData, sizeof(debugData));
    // GPIO_WriteBit(GPIOC, GPIO_Pin_0, Bit_RESET);
#elif UART_DEBUG_ENABLE == 1
    DrvUartSendBytes((uint8_t *)&debugData, sizeof(debugData));
#else
#ifdef USB_DEBUG
    Deal_sendMonitorData((uint8_t *)&debugData, sizeof(debugData));
#endif
#endif
}

void sdiOut(uint8_t *buf, int size) {
    int writeSize = size;
    int i = 0;

    do
    {

        /**
         * data0 + data1 = 8 bytes
         * data0 The lowest byte storage length, the maximum is 7
         *
         */

        while( (*(DEBUG_DATA0_ADDRESS) != 0u))
        {

        }

        if(writeSize>7)
        {
            *(DEBUG_DATA1_ADDRESS) =      \
              ((uint8_t)(buf[i+3])) |     \
              ((uint8_t)(buf[i+4]) << 8) |  \
              ((uint8_t)(buf[i+5]) << 16) | \
              ((uint8_t)(buf[i+6]) << 24);
            *(DEBUG_DATA0_ADDRESS) =      \
              (7u) |                      \
              ((uint8_t)(buf[i]) << 8) |    \
              ((uint8_t)(buf[i+1]) << 16) | \
              ((uint8_t)(buf[i+2]) << 24);   

            i += 7;
            writeSize -= 7;
        }
        else
        {
            uint8_t tmp = (uint8_t)writeSize;
            *(DEBUG_DATA1_ADDRESS) =                       \
                ((tmp > 3 ? (uint8_t)buf[i+3] : 0)) |      \
                ((tmp > 4 ? (uint8_t)buf[i+4] : 0) << 8) | \
                ((tmp > 5 ? (uint8_t)buf[i+5] : 0) << 16) |\
                ((tmp > 6 ? (uint8_t)buf[i+6] : 0) << 24);
            *(DEBUG_DATA0_ADDRESS) = 
                (tmp) | ((uint8_t)buf[i] << 8) |           \
                ((tmp > 1 ? (uint8_t)buf[i+1] : 0) << 16) |\
                ((tmp > 2 ? (uint8_t)buf[i+2] : 0) << 24);
            writeSize = 0;
        }

    } while (writeSize);
}
