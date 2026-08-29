/********************************** (C) COPYRIGHT  *******************************
 * File Name          : drivers.c
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

// DriversInit 初始化
void DriversInit(void) {
    SystemInit();
    DrvTime1Init();
    DrvAdcInit();
    DrvKeyInit();
    DrvUartInit();
    DrvTime2Init();
    DrvSystickInit();
}
