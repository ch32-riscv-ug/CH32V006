/********************************** (C) COPYRIGHT  *******************************
 * File Name          : control.h
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
#ifndef _CONTROL_H
#define _CONTROL_H

#include "def.h" // IWYU pragma: export


extern volatile uint32_t gMotorIsNeedRun;
extern volatile int32_t gMultiSamplingPoint[2];
extern volatile uint32_t gMotorControlEpoch;

extern void MotorControl(void);
extern void EnterRunInit(void);
extern void MotorInit(void);

// Clear the speed and desync statistics of the startup phase; shared by forced drag and head/tail-wind entry into closed loop.
void MotorResetRunFeedback(void);

// Restore the advance angle and invalidate in-flight outputs; the parameter is kept for interface compatibility.
void MotorResetControl(bool resetLoop);

// A fault/emergency stop shuts down immediately from any state; it does not depend on the ADC interrupt continuing to run.
void MotorEmergencyStop(uint16_t errorMask);

// Publish the closed-loop controller initial values; the main loop PI state is not modified inside the interrupt.
void MotorEnterRun(void);

void MotorZeroCross(void);

// MotorDrag: drive the motor (forced drag)
void MotorDrag(void);

// Main-loop only: publish the run duty cycle, applied after being consumed by the interrupt, without disabling interrupts.
void MotorSetDuty(int32_t q15duty);

// ADC startup state machine only: uses a mailbox separate from the main loop and must not be called from the main loop.
void MotorSetStartupDuty(int32_t q15duty);

// MotorWhile: motor main loop
void MotorWhile(void);

// MotorGetDuty: get the current duty cycle
int32_t MotorGetDuty(void);

// MotorStop: stop the motor
// brakeTimeMs: brake time, unit ms
//   - 0:  stop immediately (all PWM off, free coast down by friction/inertia)
//   - >0: the main loop publishes a dissipative brake request, executed on the next ADC interrupt.
//         All high-side off + all three low-side on, winding hard shorted.
//         Note: this is a hard short rather than a chopper, the duty registers have no effect and the braking current is uncontrolled.
//         The fault path always shuts down immediately; the non-zero parameter of this interface is only for explicitly requested normal braking.
void MotorStop(uint32_t brakeTimeMs);

// Main-loop only: publish a start request handled by the next ADC interrupt; among unconsumed requests only the latest is kept.
void MotorStart(void);

// Return the run request already accepted by the state machine, excluding start requests still pending in the mailbox.
bool MotorIsRun(void);

// MotorSpeedUpdate: update the speed-related parameters
void MotorSpeedUpdate(void);

// MotorLoop: motor loop
void MotorLoop(void);

// ADC-dominant immediate-update single mechanism: low-frequency entry rebuilds the heartbeat (bridge arms already off), delay muting, resume
void MotorAdcSamplingReset(void);
FAST_FUNC bool MotorAdcDelayArm(uint32_t when);
FAST_FUNC void MotorAdcSamplingResume(void);

#endif
