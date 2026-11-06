/********************************** (C) COPYRIGHT  *******************************
 * File Name          : config.h
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
#ifndef CONFIG_H
#define CONFIG_H

#define FAST_FUNC __attribute__((section(".highcode")))

#define UART_DEBUG_ENABLE 0
#define SDI_DEBUG_ENABLE 1

#define SYSTEM_CLOCK_FREQ 48000000uL // System clock frequency (unit: Hz)

// Independent watchdog: no drvwdg driver in this project, disabled
#define IWDG_ENABLE 0

#define PWM_FRE (20000) // PWM frequency
#define PWM_FRE_SETATA (SYSTEM_CLOCK_FREQ / PWM_FRE - 1) // Default PWM period register value

// Duty cycle definitions (Q15 per-unit, _IQ15(1.0)=32768=100%)
#define PWM_DUTYCYCLE_95 _IQ15(0.95f) // PWM duty cycle  95%
#define PWM_DUTYCYCLE_75 _IQ15(0.75f) // PWM duty cycle  75%
#define PWM_DUTYCYCLE_50 _IQ15(0.50f) // PWM duty cycle  50%
#define PWM_DUTYCYCLE_25 _IQ15(0.25f) // PWM duty cycle  25%
#define PWM_DUTYCYCLE_20 _IQ15(0.20f) // PWM duty cycle  20%
#define PWM_DUTYCYCLE_15 _IQ15(0.15f) // PWM duty cycle  15%
#define PWM_DUTYCYCLE_10 _IQ15(0.10f) // PWM duty cycle  10%
#define PWM_DUTYCYCLE_05 _IQ15(0.05f) // PWM duty cycle  5%
#define PWM_DUTYCYCLE_01 _IQ15(0.01f) // PWM duty cycle  1%

// Minimum duty cycle
#define PWM_MIN_DUTY _IQ15(0.10)
// Maximum duty cycle
#define PWM_MAX_DUTY _IQ15(1)

// Low-side precharge time, unit: us
#define LOW_BRIDGE_CHARGE_TIME 5000
// Freewheeling avoidance time, unit: us
#define AVOID_CHARGING_TIME 25
// Sampling time, unit: us
// Injected sequence is fixed to U/V/W/OPA four channels: 24MHz ADC, 20 cycles per channel, about 3.3us
#define SAMPLING_TIME 3.3f
// Switching avoidance time, unit: us
#define AVOID_SWITCH_TIME 3
// Synchronous freewheeling switch
#define SYNC_CHARGING_SWITCH 0
// Commutation delay compensation, unit: us
#define PHASE_DELAY_COMPENSATION 50
// Interval between multiple samples, unit: us
#define SAMPLING_INTERVAL 20.0f
// ADC scheme high-speed support
#define ADC_SUPPORT_HIGH_SPEED 1
// Commutation delay enable
#define PHASE_DELAY_ENABLE 1
// Comparator support: this board has no CMP zero-crossing circuit, ADC scheme only (CMP2 is used for OPA hardware over-current brake only)
#define CMP_SUPPORT_ENABLE 0

/********************Motor parameter per-unit values********************/
// Maximum supported current, A
#define CURRENT_MAX 125.0f
// Maximum supported voltage, V
#define VOLTAGE_MAX 17.78f

// Motor running parameters
// Maximum speed, unit: electrical RPM (eRPM)
#define SPEED_MAX 80000
// Minimum speed, unit: electrical RPM (eRPM)
#define SPEED_MIN 2000
/*************************************************************/

/***************************Startup parameters***************************/
// Alignment duty cycle
#define ALIGN_PWM_DUTY _IQ15(0.15)
// Alignment count
#define ALIGN_CNT 1
// Alignment time, unit: ms
#define ALIGN_TIME 0
// Number of zero-crossing detections at startup, recommended value: 3
#define STARTUP_ZCP_CNT 6
// Startup detection threshold, range: 0~100, recommended value: 5
#define STARTUP_ZERO_THRESHOLD 5
// Maximum forced-drag voltage, unit: V
// Note: the new shared library drags in zero-crossing-follow mode, so the rotor must be
// pushed open-loop above SPEED_MIN (2000 eRPM) before handing over to closed loop;
// 2V (about the old 15% duty cycle) usually cannot drive a load, R0 verified a 4~6V ramp.
// On this board start with 3~5V and tune it on hardware.
#define DRAG_VOLTAGE_MAX 5.0f
// Minimum forced-drag voltage, unit: V
#define DRAG_VOLTAGE_MIN 3.0f
// Drag ramp-up time, unit: ms (time for the duty cycle to rise linearly from DRAG_VOLTAGE_MIN to DRAG_VOLTAGE_MAX)
#define DRAG_RAMP_TIME 100
// Maximum startup time, unit: ms
#define START_TIMEOUT 1000
// Drag stall step-up / closed-loop stall time, unit: ms (early zero-crossing intervals may be 20~30ms, leave enough margin to prevent zcpCnt from being cleared)
#define STALL_TIME 30
// Head/tail-wind startup voltage threshold, unit: V (R0=0.05; 1.0V would misjudge coasting rotation as standstill)
#define WARMBOOT_VOLTAGE_THRESHOLD 0.05f
// BEMF continuous-disappearance confirmation time, unit: ms
#define WARMBOOT_TIMEOUT 50
// Reverse-wind handling mode 0: short-circuit brake then start as standstill (keep target direction) 1: follow the wind direction directly into closed loop
#define WARMBOOT_REVERSE_MODE 0
// Total reverse-wind brake timeout, unit: ms, report stall and stop if reverse wind is still detected
#define WARMBOOT_REVERSE_TIMEOUT 1000
// Short-circuit brake duration per reverse-wind round, unit: ms
#define REVERSE_BRAKE_ON_MS 20
// Time to wait for winding current decay with all switches off after reverse-wind brake, unit: ms
#define REVERSE_DEMAG_MS 2
// Holding time to keep short circuit after the reverse BEMF disappears, unit: ms
#define REVERSE_HOLD_MS 200
/*************************************************************/

/********************Static pulse injection parameters********************/
// Enable static pulse injection (detect rotor standstill position before alignment); parameters must still be provided to shared main.c when disabled
#define STATIC_PULSE_INJECT_ENABLE 0
// Injection PWM duty cycle (large duty for a short time to prevent motor rotation)
#define STATIC_PULSE_INJECT_DUTY _IQ15(1)
// Demagnetization time after each PWM turn-off, unit: us
#define STATIC_PULSE_DEMAG_TIME_US 50
// Settling time to wait for current stabilization after PWM injection, unit: us
#define STATIC_PULSE_SETTLE_TIME_US 50
// Timeout of software-triggered ADC injected sampling, unit: us
#define STATIC_PULSE_ADC_TIMEOUT_US 50
// Phase-loss detection current threshold, unit: raw ADC value
#define STATIC_PULSE_PHASE_LOSS_THRESHOLD 10
// Short-circuit detection current threshold, unit: raw ADC value
#define STATIC_PULSE_SHORT_THRESHOLD 2000
/*************************************************************/

// Desync protection threshold, unit: us (the shared library desync criterion is currently disabled, definition kept)
#define DESYNC_TIME 2000

/****************************Speed loop***************************/
// Speed loop enable
#define SPEED_LOOP_ENABLE 1
// Speed loop PI parameters, unit: float
#define SPEED_KP 0.01f
#define SPEED_KI 0.005f
// Duty cycle output range, unit: float, range 0~1;
#define SPEED_OUT_MAX 1.0f
#define SPEED_OUT_MIN 0.15f
// Speed first-order IIR filter coefficient (larger follows faster with more noise)
// Group delay = (1-a)/a commutations: 0.1->9 times, 0.25->3 times, 0.4->1.5 times
#define SPEED_IIR_ALPHA 0.4f
/************************************************************/

/***************************Current limit loop***************************/
// Current loop enable
#define CURRENT_LOOP_ENABLE 0
// Default current limit, unit: A (current limit loop disabled, must not exceed OVER_CURRENT2 when enabled)
#define CURRENT_LIMIT 8.0f
// Current limit loop PI parameters
#define CURRENT_KP 0.5f
#define CURRENT_KI 0.01f
// Output range, unit: per-unit
#define CURRENT_OUT_MAX 1.0f
#define CURRENT_OUT_MIN 0.15f
/************************************************************/

/***************************Power loop***************************/
// Power loop enable
#define POWER_LOOP_ENABLE 0
// Default target power, unit: W (power loop disabled, must be reset to the machine power before enabling)
#define POWER_TARGET 20.0f
// Power loop PI parameters
#define POWER_KP 2.0f
#define POWER_KI 0.2f
// Output range, unit: per-unit
#define POWER_OUT_MAX 1.0f
#define POWER_OUT_MIN 0.15f
/************************************************************/

/***************************Protection parameters***************************/
// Software protection enable (over-voltage / under-voltage / soft over-current)
#define PROTECT_ENABLE 1
// Over-current protection 1, unit: A
#define OVER_CURRENT1 10
// Over-current protection 1 duration, unit: ms
#define OVER_CURRENT1_TIME 200
// Over-current protection 2, unit: A
#define OVER_CURRENT2 15
// Over-current protection 2 duration, unit: ms
#define OVER_CURRENT2_TIME 50
// Over-voltage, unit: V
#define OVER_VOLTAGE 15
// Under-voltage, unit: V
#define UNDER_VOLTAGE 6
// Voltage protection time
#define VOLTAGE_PROTECT_TIME 200
// Motor brake time, unit: ms
#define BRAKE_TIME 5000
/***********************************************************/

/********************Auto-calculated parameters, do not modify******************/
#define SAMPLING_TIME_CNT ((uint32_t)((SAMPLING_TIME) * ((float)SYSTEM_CLOCK_FREQ / 1000000.0f)))
#define AVOID_CHARGING_CNT ((uint32_t)(((uint64_t)SYSTEM_CLOCK_FREQ * (uint64_t)(AVOID_CHARGING_TIME)) / 1000000uL))
#define SAMPLING_INTERVAL_CNT ((uint32_t)((SAMPLING_INTERVAL) * ((float)SYSTEM_CLOCK_FREQ / 1000000.0f)))
#define AVOID_SWITCH_CNT ((uint32_t)(((uint64_t)SYSTEM_CLOCK_FREQ * (uint64_t)(AVOID_SWITCH_TIME)) / 1000000uL))
#define PHASE_DELAY_COMPENSATION_CNT ((uint32_t)(((uint64_t)SYSTEM_CLOCK_FREQ * (uint64_t)(PHASE_DELAY_COMPENSATION)) / 1000000uL))
#define STALL_TIME_CNT ((uint32_t)((STALL_TIME) * ((float)SYSTEM_CLOCK_FREQ / 1000.0f)))
#define DESYNC_TIME_CNT ((uint32_t)((DESYNC_TIME) * ((float)SYSTEM_CLOCK_FREQ / 1000000.0f)))
/***********************************************************/

#endif
