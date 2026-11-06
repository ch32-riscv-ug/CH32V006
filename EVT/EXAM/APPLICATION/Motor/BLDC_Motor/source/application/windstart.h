/********************************** (C) COPYRIGHT  *******************************
 * File Name          : windstart.h
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
#ifndef WINDSTART_H
#define WINDSTART_H

#include "def.h"

// Reset this head/tail-wind startup and restore the target direction.
void WindStartReset(void);

// Restore the target direction from before reverse-wind following on stop or restart.
void WindStartRestoreDirection(void);

// Run the mcAhead detection; shares the control-layer timestamp to keep the timing continuity of the following align/drag stages.
void WindStartCheck(uint64_t *stateStartMs);

// Perform the mcReverse brake, demagnetization and re-check.
int WindStartReverse(uint64_t *stateStartMs);

#endif
