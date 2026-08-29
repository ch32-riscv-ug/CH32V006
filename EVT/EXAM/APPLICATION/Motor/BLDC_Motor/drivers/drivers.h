/********************************** (C) COPYRIGHT  *******************************
 * File Name          : drivers.h
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2026/08/05
 * Description        : This file contains all the functions prototypes for the
 *                       firmware library.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef DRIVERS_H
#define DRIVERS_H

#include "def.h"
#include "drvadc.h"
#include "drvtime1.h"
#include "system_ch32v00X.h"
#include "drvkey.h"
#include "drvuart.h"
#include "drvtime2.h"
#include "drvsystick.h"

// DriversInit 初始化
void DriversInit(void);

#endif
