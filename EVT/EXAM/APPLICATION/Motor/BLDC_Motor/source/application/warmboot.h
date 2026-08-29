/********************************** (C) COPYRIGHT  *******************************
 * File Name          : warmboot.h
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
#ifndef WARMBOOT_H
#define WARMBOOT_H

#include "def.h"

// WarmBootInit 初始化顺风启动
void WarmBootInit(float bemfThreshold);

// WarmBootCheck 顺逆风检测是否过零
bool WarmBootCheck(void);

// WarmBootGetSector 获取换相扇区
uint8_t WarmBootGetSector(void);

// WarmBootIsOK 检测是否检测到6次过零
bool WarmBootIsOK(void);

// WarmBootGetIsBemf 获取是否检测到BEMF
bool WarmBootGetIsBemf(void);

// WarmBootReset 重置顺风启动
void WarmBootReset(void);

#endif

