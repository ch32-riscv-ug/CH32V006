/********************************** (C) COPYRIGHT  *******************************
 * File Name          : adcsample.h
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
#ifndef ADCSAMPLE_H
#define ADCSAMPLE_H

#include <stdbool.h>
#include <stdint.h>
#include "def.h"

// AdcSampleLoop: ADC cyclic sampling of voltage and current
void AdcSampleLoop(void);

#endif
