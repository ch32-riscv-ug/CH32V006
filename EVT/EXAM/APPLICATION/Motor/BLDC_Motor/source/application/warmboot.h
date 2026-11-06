/********************************** (C) COPYRIGHT  *******************************
 * File Name          : warmboot.h
 * Author             : WCH
 * Version            : V1.0.1
 * Date               : 2026//05
 * Description        : This file contains all the functions prototypes for the
 *                       firmware library.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef WARMBOOT_H
#define WARMBOOT_H

#include "def.h"

// WarmBootInit: initialize head-wind startup
void WarmBootInit(float bemfThreshold);

// WarmBootCheck: check whether head/tail-wind detection sees a zero crossing
bool WarmBootCheck(void);

// WarmBootGetSector: get the commutation sector
uint8_t WarmBootGetSector(void);

// WarmBootIsOK: check whether the consecutive valid commutation count is reached
bool WarmBootIsOK(void);

// WarmBootGetIsBemf: get whether BEMF has been detected
bool WarmBootGetIsBemf(void);

// WarmBootGetDirection: get the detected rotation direction
uint8_t WarmBootGetDirection(void);

// WarmBootReset: reset head-wind startup
void WarmBootReset(void);

#endif

