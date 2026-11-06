/********************************** (C) COPYRIGHT  *******************************
 * File Name          : comparator.h
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

#ifndef COMPARATOR_H
#define COMPARATOR_H

#include "framework.h"

// Get the comparator output level
typedef bool (*CmpGet)(void);

// ComparatorInit: comparator initialization
// fun : function that gets the comparator output
void ComparatorInit(CmpGet fun);

// ComparatorTask: comparator task
void ComparatorTask(void);

// ComparatorReset: reset the comparator state
void ComparatorReset(void);

// ComparatorPollingStart: start comparator polling mode
void ComparatorPollingStart(void);

// ComparatorPollingStop: stop comparator polling mode
void ComparatorPollingStop(void);

// ComparatorIsPolling: whether polling mode is active
bool ComparatorIsPolling(void);

// ComparatorIsFoundZeroCross: whether a zero crossing has been found
bool ComparatorIsFoundZeroCross(void);

// ComparatorAcknowledgeZeroCross: called by the upper layer after handling the zero-crossing commutation, resets the BEMF detection state
void ComparatorAcknowledgeZeroCross(void);

// ComparatorInterruptRoutine: comparator interrupt handler
void ComparatorInterruptRoutine(void);

#endif
