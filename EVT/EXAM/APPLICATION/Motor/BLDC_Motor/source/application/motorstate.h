/********************************** (C) COPYRIGHT  *******************************
 * File Name          : motorstate.h
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
#ifndef MOTORSTATE_H
#define MOTORSTATE_H

// Called from the ADC control interrupt entry; ordinary start/stop requests modify the state machine only in this context.
void MotorStateProcessRequests(void);

// Only handles non-mcRun states; in steady state the interrupt runs the inline run logic of control.c directly.
void MotorStateRun(void);

#endif
