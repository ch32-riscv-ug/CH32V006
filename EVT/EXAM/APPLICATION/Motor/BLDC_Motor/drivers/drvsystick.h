/********************************** (C) COPYRIGHT  *******************************
 * File Name          : drvsystick.h
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
#ifndef DRVSYSTICK_H
#define DRVSYSTICK_H

#include "def.h"

// DrvSystickInit: driver initialization
void DrvSystickInit(void);

// DrvSystickCmd: enable or disable the systick timer
// enable: enable flag
// CNT is set to 0 whether enabled or disabled
void DrvSystickCmd(bool enable);


// DrvSystickClear: clear the systick counter
void DrvSystickClear(void);

#endif
