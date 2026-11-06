/********************************** (C) COPYRIGHT  *******************************
 * File Name          : param.h
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

#ifndef PARAM_H
#define PARAM_H

#include "interface.h" // IWYU pragma: export

#define BLDC_LIB_VERSION "1.0.0"

// Direction
#define CW 0
#define CCW 1

typedef enum {
    ADC_SENSOR = 0,
    CMP_SENSOR = 1,
    ADC_CMP_SENSOR = 2,
    HALL_SENSOR = 3,
} SensorType_T;

typedef enum {
    U_PHASE = 0,
    V_PHASE,
    W_PHASE,
} Phase_T;

// Control sequence
typedef enum {
    mcStop = 0, // Motor stopped
    mcIdle,
    mcCharge,
    mcAhead, // Startup detection
    mcPulse, // Static pulse injection alignment
    mcAlign, // Alignment
    mcDrag,  // Forced-drag startup
    mcRun,   // Enter closed loop
    mcdec,   // Deceleration
    mcHigh,
    mcReset,   // Reset
    mcFault,   // Motor restart
    mcReverse, // Reverse-wind braking
} MotorState_T;

// Printed error information
typedef union {
    struct {
        uint16_t OverCurrent1 : 1;   // Over-current protection 1
        uint16_t OverCurrent2 : 1;   // Over-current protection 2
        uint16_t OverCurrentCMP : 1; // Over-current protection comparator
        uint16_t OverVoltage : 1;    // Over-voltage
        uint16_t UnderVoltage : 1;   // Under-voltage
        uint16_t Stall : 1;          // Stall
        uint16_t Desync : 1;         // Desync
        uint16_t PhaseLoss : 1;      // Phase loss
        uint16_t PhaseShort : 1;     // Short circuit
        uint16_t TempOver : 1;       // Over-temperature
        uint16_t SampleTimeout : 1;  // Sampling chain timeout
        uint16_t ControlTimeout : 1; // Main loop timeout
    } bit;
    uint16_t value;
} ErrorState_T;

typedef enum {
    ERROR_MASK_OVER_CURRENT1 = 1u << 0,
    ERROR_MASK_OVER_CURRENT2 = 1u << 1,
    ERROR_MASK_OVER_CURRENT_CMP = 1u << 2,
    ERROR_MASK_OVER_VOLTAGE = 1u << 3,
    ERROR_MASK_UNDER_VOLTAGE = 1u << 4,
    ERROR_MASK_STALL = 1u << 5,
    ERROR_MASK_DESYNC = 1u << 6,
    ERROR_MASK_PHASE_LOSS = 1u << 7,
    ERROR_MASK_PHASE_SHORT = 1u << 8,
    ERROR_MASK_TEMP_OVER = 1u << 9,
    ERROR_MASK_SAMPLE_TIMEOUT = 1u << 10,
    ERROR_MASK_CONTROL_TIMEOUT = 1u << 11,
} ErrorMask_T;

// The stall bit allows drag recovery; all other faults forbid committing drive outputs.
#define ERROR_MASK_BLOCKING ((uint16_t)(0x0FFFu & ~ERROR_MASK_STALL))

// Motor direction, duty cycle and other parameters
typedef struct
{
    uint32_t RotorDirection;        // Motor rotation direction
    int32_t PWMDutyCycle;           // PWM duty cycle
    int32_t Phase;                  // Phase
    int32_t Speed;                  // Actual speed
    int32_t SpeedFilter;            // Filtered speed
    int32_t SetSpeed;               // Set speed
    uint32_t SpeedTime_Sum;         // Sum of 6 commutation times
    uint32_t SpeedTime_Cnt;         // Commutation average counter
    uint32_t ZeroCrossTime;         // Zero-crossing time
    uint32_t ZeroCrossTimeTable[6]; // Zero-crossing time table
    int32_t ZeroCrossTimeSum;       // Zero-crossing time sum
    int32_t LastZeroCrossTimeSum;   // Previous zero-crossing time sum
    uint32_t LastZeroCrossTime;
    uint32_t ChangePhaseTimestamp;
    uint32_t PhaseDelayCompensation; // Commutation delay compensation: PHASE_DELAY_COMPENSATION_CNT in ADC mode, IC filter delay in CMP mode
    uint32_t PhaseIsChange;
    SensorType_T SensorType;
    uint32_t IsCmpMode;
    uint32_t IsSyncChargingMode; // Complementary (synchronous freewheeling) mode switch: true=complementary, false=normal
    uint32_t DesyncCheck;
    uint32_t ControlLoopCnt;
    uint32_t Count; // Control loop counter
} HoldControlPara_T;

// ADC related
typedef struct
{
    uint16_t CurrentOffset;   // Current amplifier zero point
    uint32_t CalibrationIsOK; // Whether calibration is complete
    int32_t CalibrationSum;  // Calibration sum
    uint32_t CalibrationCnt;  // Calibration count
    uint32_t POT;             // Potentiometer
    uint32_t Voltage;         // Voltage sample value
    int16_t Current;          // Current sample value
    int16_t CurrentFilter;    // Filtered current value
    uint16_t BemfPhase;       // BEMF detection phase
    uint16_t OnPhaseHalf;     // Half of the conducting phase voltage
    uint16_t BemfRef;         // BEMF reference value
    uint16_t PhaseVoltage[3]; // Three-phase voltages, order 0-U, 1-V, 2-W
    uint16_t NeutralPoint;    // BEMF neutral point
    uint16_t Ntc;             // Temperature sample value
    uint16_t ExAdc1;          // Extended ADC1
    uint16_t ExAdc2;          // Extended ADC2
    uint32_t Power;           // Power Q24
    uint32_t SampleErr;       // Sampling error
} ADCSamplePara_T;

extern volatile MotorState_T mcState;
extern volatile ErrorState_T ErrorCode;
extern volatile HoldControlPara_T HoldParm;
extern volatile ADCSamplePara_T ADCSample;

// ParamLoad: parameter loading
void ParamLoad(void);

// ParamReset: parameter reset
void ParamReset(void);

#endif
