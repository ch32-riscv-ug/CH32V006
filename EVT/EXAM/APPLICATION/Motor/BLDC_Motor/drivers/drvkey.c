/********************************** (C) COPYRIGHT  *******************************
 * File Name          : drvkey.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2026/08/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "drivers.h"

// DrvKeyInit 按键初始化
void DrvKeyInit(void) {

    GPIO_InitTypeDef GPIO_InitStructure = {0};
    RCC_PB2PeriphClockCmd(RCC_PB2Periph_GPIOD | RCC_PB2Periph_AFIO,
                          ENABLE);

    // PWM用IO配置
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = DRVKEY_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 上管PWM
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(DRVKEY_PORT, &GPIO_InitStructure);
}

// DrvKeyGet 按键获取
bool DrvKeyGet(void) {
    return GPIO_ReadInputDataBit(DRVKEY_PORT, DRVKEY_PIN) == Bit_RESET;
}
