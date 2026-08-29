/********************************** (C) COPYRIGHT  *******************************
 * File Name          : config.h
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
#ifndef CONFIG_H
#define CONFIG_H

#define VERSION "1.0.2"

#define FAST_FUNC __attribute__((section(".highcode")))

#define UART_DEBUG_ENABLE 0
#define SDI_DEBUG_ENABLE 0

// 方向
#define CW 0
#define CCW 1

#define SYSTEM_CLOCK_FREQ 48000000uL // 系统时钟频率(unit-Hz)

#define PWM_FRE (20000) // PWM的频率
#define PWM_FRE_SETATA (SYSTEM_CLOCK_FREQ / PWM_FRE - 1)
#define PWM_DUTYCYCLE_95 ((PWM_FRE_SETATA / 20) * 19) // PWM的占空比  95%
#define PWM_DUTYCYCLE_75 ((PWM_FRE_SETATA / 4) * 3)   // PWM的占空比  75%
#define PWM_DUTYCYCLE_50 (PWM_FRE_SETATA / 2)         // PWM的占空比  50%
#define PWM_DUTYCYCLE_25 (PWM_FRE_SETATA / 4)         // PWM的占空比  25%
#define PWM_DUTYCYCLE_20 (PWM_FRE_SETATA / 5)         // PWM的占空比  20%
#define PWM_DUTYCYCLE_15 ((PWM_FRE_SETATA / 20) * 3)  // PWM的占空比  15%
#define PWM_DUTYCYCLE_10 (PWM_FRE_SETATA / 10)        // PWM的占空比  10%
#define PWM_DUTYCYCLE_05 (PWM_FRE_SETATA / 20)        // PWM的占空比  5%
#define PWM_DUTYCYCLE_01 (PWM_FRE_SETATA / 100)

#define PWM_START_DUTY (PWM_DUTYCYCLE_10)
#define PWM_MIN_DUTY (PWM_DUTYCYCLE_10) // 最小占空比
#define PWM_MAX_DUTY PWM_FRE_SETATA     // 最大占空比

// 下管预充电时间 单位: us
#define LOW_BRIDGE_CHAGRE_TIME 5000
// 规避续流时间 单位:us
#define AVOID_CHARGING_TIME 25
// 采样时间 单位:us
// 需要根据硬件电路进行分析
// 需要结合PWM
#define SAMPLING_TIME 2.0f
// 开关避开时间 单位:us
#define AVOID_SWITCH_TIME 3
// 同步续流开关
#define SYNC_CHARGING_SWITCH 0
// 换相delay补偿 单位运行周期
#define PHASE_DELAY_COMPENSATION (50 * 48)
// 多次采样中间间隔 单位:us
#define SAMPLING_INTERVAL 15.0f
// ADC方案高速支持
#define ADC_SUPPORT_HIGH_SPEED 1
// 换相延时使能
#define PHASE_DELAY_ENABLE 1
/*************************电机参数标幺值*************************/
// 最大支持电流 A
#define CURRENT_MAX 125.0f
// 最大支持电压 V
#define VOLTAGE_MAX 17.78f

// 电机运行参数
// 最大转速 单位:电角度 eRPM
#define SPEED_MAX 16000
// 最低转速 单位:电角度 eRPM
#define SPEED_MIN 2000
/*************************************************************/

/***************************启动参数***************************/
// 定位占空比
#define ALIGN_PWM_DUTY PWM_DUTYCYCLE_15
// 定位次数
#define ALIGN_CNT 1
// 定位时间 单位:ms
#define ALIGN_TIME 0
// 启动检测过零点次数 推荐默认值:3
#define STARTUP_ZCP_CNT 3
// 启动检测阈值 阈值:0~100 推荐默认值:5
#define STARTUP_ZERO_THRESHOLD 5
// 强托起始速度 单位:电角度 eRPM
#define DRAG_SPEED_MIN 500
// 强托最大速度 单位:电角度 eRPM
#define DRAG_SPEED_MAX 1000
// 强托占空比最大值
#define DRAG_PWM_DUTY_MAX PWM_DUTYCYCLE_15
// 强托占空比最小值
#define DRAG_PWM_DUTY_MIN PWM_DUTYCYCLE_15
// 启动最大时间, 单位ms
#define START_STALL_TIME 3000
// 顺逆风启动电压阈值 单位:V
#define WARMBOOT_VOLTAGE_THRESHOLD 1.0f
// 顺逆风超时时间 单位:ms
#define WARMBOOT_TIMEOUT 50
/*************************************************************/

/****************************速度环***************************/
// 速度环PI参数 单位:float
#define SPEED_KP 0.01f
#define SPEED_KI 0.005f
// 占空比输出范围 单位:float 范围0~1;
#define SPEED_OUT_MAX 1.0f
#define SPEED_OUT_MIN 0.15f
/************************************************************/

/***************************保护参数***************************/
// 过流保护1 单位:A
#define OVER_CURRENT1 5
// 过流保护1持续时间 单位:ms
#define OVER_CURRENT1_TIME 200
// 过流保护2 单位:A
#define OVER_CURRENT2 8
// 过流保护2持续时间 单位:ms
#define OVER_CURRENT2_TIME 50
// 过压 单位:V
#define OVER_VOLTAGE 15
// 欠压 单位:V
#define UNDER_VOLTAGE 8
// 电压保护时间
#define VOLTAGE_PROTECT_TIME 200
// 闭环堵转时间 单位:ms
#define STALL_TIME 100
// 缺项保护阈值 低于该值则触发缺项保护
// ADC采样值，通常很小
#define PHASE_LOSS_THRESHOLD 25
// 电机刹车时间 单位:ms
#define BRAKE_TIME 5000
/***********************************************************/

/********************参数自动计算，请不要修改******************/
#define DRAG_CNT_MIN ((uint32_t)(PWM_FRE * 60 / DRAG_SPEED_MAX / 6))
#define DRAG_CNT_MAX ((uint32_t)(PWM_FRE * 60 / DRAG_SPEED_MIN / 6))
#define SAMPLING_TIME_CNT ((uint32_t)((SAMPLING_TIME / (1e6 / PWM_FRE)) * PWM_FRE_SETATA))
#define AVOID_CHARGING_CNT ((uint32_t)((AVOID_CHARGING_TIME / (1e6 / PWM_FRE)) * PWM_FRE_SETATA))
#define SAMPLING_INTERVAL_CNT ((uint32_t)((SAMPLING_INTERVAL / (1e6 / PWM_FRE)) * PWM_FRE_SETATA))
#define AVOID_SWITCH_CNT ((uint32_t)((AVOID_SWITCH_TIME / (1e6 / PWM_FRE)) * PWM_FRE_SETATA))
/***********************************************************/

#endif
