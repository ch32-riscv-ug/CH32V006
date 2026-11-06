/********************************** (C) COPYRIGHT  *******************************
 * File Name          : drivers.c
 * Author             : WCH
 * Version            : V1.0.1
 * Date               : 2026/11/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "drivers.h"

// DriversInit: initialization
void DriversInit(void) {
    SystemInit();
    DrvTime1Init();
    DrvAdcInit();
    DrvKeyInit();
    DrvUartInit();
    DrvSystickInit();
}
