/********************************** (C) COPYRIGHT  *******************************
 * File Name          : common.h
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
#ifndef _COMMON_H
#define _COMMON_H

#include "drivers.h"
#include "stddef.h"
#include "stdint.h"

typedef enum {
    U_PHASE = 0,
    V_PHASE,
    W_PHASE,
} Phase_T;

// 控制顺序
typedef enum {
    mcStop = 0, // 电机停止工作
    mcIdle,
    mcCharge,
    mcAhead, // 启动检测
    mcAlign, // 定位
    mcDrag,  // 强拖启动
    mcRun,   // 进入闭环
    mcdec,   // 减速
    mcHigh,
    mcReset, // 重置
    mcFault, // 电机重启
} MotorState_T;

// 打印错误信息
typedef union {
    struct {
        uint16_t OverCurrent1 : 1;   // 过流保护1
        uint16_t OverCurrent2 : 1;   // 过流保护2
        uint16_t OverCurrentCMP : 1; // 过流保护比较器
        uint16_t OverVoltage : 1;    // 过压
        uint16_t UnderVoltage : 1;   // 欠压
        uint16_t Stall : 1;          // 堵转
        uint16_t SpeedError : 1;     // 失步
        uint16_t PhaseLoss : 1;      // 缺项
    } bit;
    uint16_t value;
} ErrorState_T;

// 电机运行方向、占空比等参数
typedef struct
{
    uint8_t RotorDirection; // 电机转动方向
    uint8_t MainDetectCnt;  // 主循环计数
    uint16_t SpeedLoopCnt;  // 调整转速占空比的周期计数
    uint16_t PWMDutyCycle;  // PWM占空比
    uint16_t SpeedTime;     // 一个电周期时间
    uint8_t Phase;          // 相位
    int32_t Speed;          // 实际转速
    int32_t SpeedFilter;    // 滤波后的转速
    int32_t SetSpeed;       // 设定转速
    uint16_t DragCnt;       // 强拖时间
    uint32_t SpeedTime_Sum; // 换相时间6次和
    uint8_t SpeedTime_Cnt;  // 换相平均数计数
    uint16_t SpeedTimeTemp; // 当前换相时间
    uint8_t Direction_Cnt;  // 顺逆风检测次数
    uint8_t Direction_OK_Cnt;
    uint32_t ZeroCrossTime; // 过零点时间
    uint32_t LastZeroCrossTime;
    uint32_t ChangePhaseTimestamp;
    uint32_t FirstZeroCrossTimestamp;
    bool PhaseIsChange;
    bool IsSupportCmp;
    bool IsCmpMode;
} HoldControlPara_T;

// ADC相关
typedef struct
{
    uint16_t CurrentOffset;  // 电流放大零点
    bool CalibrationIsOK;    // 校准是否完成
    uint32_t CalibrationSum; // 校准和
    uint16_t CalibrationCnt; // 校准次数
    uint32_t POT;            // 电位器
    uint32_t Voltage;        // 电压采样值
    int16_t Current;         // 电流采样值
    int16_t CurrentFilter;   // 电流滤波值
    uint16_t BemfPhase;      // 反电势检测相
    uint16_t OnPhase;        // 导通相
    uint16_t OnPhaseHalf;    // 导通相的一半
    uint16_t UBemf;          // 反电动势U相
    uint16_t VBemf;          // 反电动势V相
    uint16_t WBemf;          // 反电动势W相
    uint16_t NeutralPoint;   // 反电动势中性点
    uint16_t Ntc;            // 温度采样值
    uint16_t ExAdc1;         // 扩展ADC1
    uint16_t ExAdc2;         // 扩展ADC2;
} ADCSamplePara_T;

// PWM捕获相关
typedef struct
{
    uint16_t Period;      // 周期
    uint16_t Duty;        // 电平时间
    uint16_t PWMCnt;      // PWM信号滤波
    uint8_t DutyCycleUse; // 占空比
    uint8_t Flag_Cap_Valid : 1;
} PWMCatchPara_T;

typedef struct
{
    uint8_t HallessState;     // 反电动势输出
    uint8_t LastHallessState; // 上一次反电动势输出
    uint8_t BackEMFFilter;    // 反电动势滤波
    uint8_t Zero_Flag;        // 过零点标识位, 注意使用完以后需要清除
    uint16_t Check_Count;     // 检测计数，换相时间计算
    uint8_t Filter_Times;     // 滤波次数
    uint16_t Delay_Cnt;       // 延时换相时间
    bool FilterSwitch;        // 滤波开关
    uint32_t ZeroCrossTime;   // 过零点时间
    uint8_t ZeroPolarity;     // 过零点极性
} SensorPara_T;

extern volatile SensorPara_T Halless;

extern volatile MotorState_T mcState;
extern volatile ErrorState_T ErrorCode;
extern volatile PWMCatchPara_T CatchParm;
extern volatile HoldControlPara_T HoldParm;
extern volatile ADCSamplePara_T ADCSample;

extern void Common_Init(void);
extern void PowerupParaInit(void);

#endif
