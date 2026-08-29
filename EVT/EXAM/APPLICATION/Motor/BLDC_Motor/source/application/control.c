/********************************** (C) COPYRIGHT  *******************************
 * File Name          : control.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2026/08/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "control.h"
#include "adcsample.h"
#include "halless.h"
#include "isr.h"
#include "launch.h"
#include "pwm.h"
#include "speed.h"
#include "startup.h"
#include "warmboot.h"

#define BRAKE_INTERVAL_TIME 5

volatile static int32_t gDelayChargePhaseTime = 0;

volatile bool gMotorIsNeedRun = false;
volatile static bool gMotorTriggered = false;
static uint64_t gStartTime = 0;

static struct pt gAlignPT = {0};

static volatile uint16_t gDuty = PWM_MIN_DUTY;

// 采样点规划方案
// 采样间隔15us
// 第一个点需要规避前2us，第二个点间隔第一个点15us，第三个点间隔第二个点15us
volatile int32_t gMultiSamplingPoint[4] = {0, 0, 0};
volatile int32_t gMultiSamplingCnt = 1;
volatile uint8_t gMultiSamplingPointNow = 0;

static volatile uint32_t gDelayZeroCrossTime = 0;

static inline void multiSamplingCalc(uint16_t duty);
static inline void calcSpeedTime(void);
static inline void delayCharge(void);
static inline void multiSampling(void);
static inline void adcCalibration(void);
static inline void idle(void);
static inline void ahead(void);
static inline void drag(void);
static inline void run(void);
static inline int alignTask(void);
static inline int charge(void);
static inline bool isCheckZeroCross(void);
static inline int brake(void);

/******************************************************************************
 函 数 名  : MotorInit
 功能描述  : 初始化
 输入参数  : 无
 输出参数  : void
******************************************************************************/
void MotorInit(void) {
    if (ErrorCode.bit.OverVoltage || ErrorCode.bit.UnderVoltage) {
        return;
    }

    Common_Init();
    SpeedReset();
    StartupReset();
    WarmBootReset();
    HallessReset();
    gAlignPT.lc = 0;
}

/*****************************************************************************
 函 数 名  : MotorAlign
 功能描述  : 定位初始相位，开启ADC中断，准备进入强拖启动
 输入参数  : 无
 输出参数  : void
*****************************************************************************/
void MotorAlign(void) {
    HoldParm.PWMDutyCycle = PWM_DUTYCYCLE_05;
    HoldParm.Phase = 0;
    // TODO 看需求来进行续流
    // TODO 看需求来进行定位
    mcState = mcDrag;
}

static inline void idle(void) {
    if (ADCSample.CalibrationIsOK == false) {
        adcCalibration();
    } else {
        if (gMotorIsNeedRun == true) {
            MotorInit();
            mcState = mcCharge;
        }
    }
}

static inline void ahead(void) {
    // 顺风检测
    if (WarmBootCheck() == true) {
        HoldParm.ZeroCrossTime = SysTick->CNT;
        calcSpeedTime();
        DrvSystickClear();
    }

    // 如果检测不到反电势则进入定位
    if (WarmBootGetIsBemf() == false || TZTimeGetMillsecond() - gStartTime > WARMBOOT_TIMEOUT) {
        mcState = mcAlign;
        HoldParm.SpeedTime_Sum = 0;
        DrvSystickCmd(false);
    }

    // 检测到顺风切入到闭环
    if (WarmBootIsOK() == true) {
        mcState = mcRun;
        HoldParm.Phase = WarmBootGetSector();
        if (++HoldParm.Phase > 5) {
            HoldParm.Phase = 0;
        }
        MotorSetDuty(PWM_DUTYCYCLE_25);
        PWMSwitchPhase();
    }
}

// 电机强制定位任务
static inline int alignTask(void) {
    static uint64_t time = 0;
    static int i = 0;

    PT_BEGIN(&gAlignPT);

    // 电机强制定位
    HoldParm.PWMDutyCycle = ALIGN_PWM_DUTY;
    HoldParm.DragCnt = DRAG_CNT_MAX;
    MotorSetDuty(ALIGN_PWM_DUTY);
    for (i = 0; i < ALIGN_CNT; i++) {
        HoldParm.Phase = i;
        PWMSwitchPhase();
        time = TZTimeGetMillsecond();
        PT_WAIT_UNTIL(&gAlignPT, TZTimeGetMillsecond() - time > ALIGN_TIME);
    }

    mcState = mcDrag;
    gStartTime = TZTimeGetMillsecond();

    PT_END(&gAlignPT);
}

static inline int charge(void) {
    static struct pt pt = {0};
    static uint64_t time = 0;
    static uint8_t i = 0;
    PT_BEGIN(&pt);
    // 下管预充电时间
    for (i = 0; i < 3; i++) {
        PWMCharge(i);
        time = TZTimeGet();
        PT_WAIT_UNTIL(&pt, TZTimeGet() - time > LOW_BRIDGE_CHAGRE_TIME);
    }

    PWMSetAllOff();
    mcState = mcAhead;
    DrvSystickCmd(true);
    gStartTime = TZTimeGetMillsecond();
    i = 0;

    PT_END(&pt);
}

static inline void drag(void) {
    if (StartupDrag() == true) {
        mcState = mcRun;
        DrvSystickCmd(true);
    }

    if (TZTimeGetMillsecond() - gStartTime > START_STALL_TIME || gMotorIsNeedRun == false) {
        if (gMotorIsNeedRun == true) {
            gMotorIsNeedRun = false;
            ErrorCode.bit.Stall = 1;
        }
        mcState = mcStop;
    }
}

static inline void run(void) {
    if (isCheckZeroCross() == true) {
        if (CheckZeroCross(HoldParm.RotorDirection, HoldParm.Phase, ADCSample.BemfPhase, ADCSample.OnPhase) == true) {
            MotorZeroCross();
#if CMP_SUPPORT_ENABLE == 1
            if (HoldParm.ZeroCrossTime < PWM_FRE_SETATA * CMP_OPEN_CNT && HoldParm.IsSupportCmp == true) {
                HoldParm.IsCmpMode = true;
            }
#endif
        } else if (StartupIsFirstZeroCrossTriggered() == true) {
            HoldParm.FirstZeroCrossTimestamp = SysTick->CNT;
        }
    }
    multiSampling();
}

FAST_FUNC void MotorZeroCross(void) {
    if (HoldParm.FirstZeroCrossTimestamp != 0) {
        HoldParm.ZeroCrossTime = SysTick->CNT;
        gDelayZeroCrossTime = HoldParm.ZeroCrossTime - HoldParm.FirstZeroCrossTimestamp;
        HoldParm.FirstZeroCrossTimestamp = 0;
    } else {
        HoldParm.ZeroCrossTime = SysTick->CNT;
        gDelayZeroCrossTime = 0;
    }
    DrvSystickClear();
    HoldParm.ChangePhaseTimestamp = 0;
    HoldParm.PhaseIsChange = false;
    if (++HoldParm.Phase > 5) {
        HoldParm.Phase = 0;
    }
    if (HoldParm.LastZeroCrossTime == 0 || PHASE_DELAY_ENABLE == 0) {
        PWMSwitchPhase(); // 换相
        SysTick->CMP = STALL_TIME * (SYSTEM_CLOCK_FREQ / 1000);
    } else {
        delayCharge();
    }
    calcSpeedTime();
    HoldParm.LastZeroCrossTime = HoldParm.ZeroCrossTime;
}

static inline bool isCheckZeroCross(void) {
    if (HoldParm.PhaseIsChange != true) {
        return false;
    }

    if (HoldParm.IsCmpMode == true) {
        return false;
    }

    uint16_t diff = 0;

    if (ADCSample.Voltage > ADCSample.OnPhase) {
        diff = ADCSample.Voltage - ADCSample.OnPhase;
    } else {
        diff = ADCSample.OnPhase - ADCSample.Voltage;
    }

    if (diff > (ADCSample.Voltage >> 2)) {
        return false;
    }

    // 换相间隔小于15度则认为是错误信号
    if ((SysTick->CNT - HoldParm.ChangePhaseTimestamp < (HoldParm.LastZeroCrossTime >> 2)) && (HoldParm.LastZeroCrossTime != 0)) {
        return false;
    }

    if (SysTick->CNT - HoldParm.ChangePhaseTimestamp < AVOID_CHARGING_CNT) {
        return false;
    }

    return true;
}

static inline int brake(void) {
    static struct pt pt = {0};
    static uint64_t time = 0;
    static int i = 0;
    static int deltaPwm = 0;

    PT_BEGIN(&pt);

    deltaPwm = (gDuty - PWM_MIN_DUTY) / (BRAKE_TIME / BRAKE_INTERVAL_TIME) + 1;

    for (i = 0; i < BRAKE_TIME / BRAKE_INTERVAL_TIME; i++) {
        gDuty -= deltaPwm;
        if (gDuty < 0) {
            gDuty = 0;
            mcState = mcStop;
            break;
        }
        time = TZTimeGetMillsecond();
        PT_WAIT_UNTIL(&pt, TZTimeGetMillsecond() - time > BRAKE_INTERVAL_TIME || gMotorIsNeedRun == true);
        if (gMotorIsNeedRun == true) {
            mcState = mcRun;
            break;
        }
    }

    i = 0;

    PT_END(&pt);
}

static inline void delayCharge(void) {
    // TODO 延迟15度高速效果更好
    gDelayChargePhaseTime = ((HoldParm.ZeroCrossTime + HoldParm.LastZeroCrossTime) >> 3) - gDelayZeroCrossTime;

    if (gDelayChargePhaseTime <= 0) {
        PWMSwitchPhase();
        SysTick->CMP = STALL_TIME * (SYSTEM_CLOCK_FREQ / 1000);
        return;
    }

    SysTick->CMP = gDelayChargePhaseTime;
}

/*****************************************************************************
 函 数 名  : threeBemfSample
 功能描述  : 三相BEMF采样
 输入参数  : 无
 输出参数  : void
*****************************************************************************/
// TODO 需要根据板子进行适配

// ADC校准
static inline void adcCalibration(void) {
    if (++ADCSample.CalibrationCnt > 8) {
        ADCSample.CurrentOffset = ADCSample.CalibrationSum >> 3;
        ADCSample.CalibrationSum = 0;
        ADCSample.CalibrationCnt = 0;
        ADCSample.CalibrationIsOK = true;
    } else {
        ADCSample.CalibrationSum += ADCSample.Current;
    }
}

// 计算采样点
// 根据占空比自动配置采样点
static inline void multiSampling(void) {
    uint16_t cnt = TIM1->CNT;

    if (cnt < gMultiSamplingPoint[1] && gMultiSamplingPoint[1] != 0) {
        AdcTriggerPoint(gMultiSamplingPoint[1]);
        gMultiSamplingPointNow = 1;
        return;
    }

    if (cnt < gMultiSamplingPoint[2] && gMultiSamplingPoint[2] != 0) {
        AdcTriggerPoint(gMultiSamplingPoint[2]);
        gMultiSamplingPointNow = 2;
        return;
    }

    if (cnt < gMultiSamplingPoint[3] && gMultiSamplingPoint[3] != 0) {
        AdcTriggerPoint(gMultiSamplingPoint[3]);
        gMultiSamplingPointNow = 3;
        return;
    }
}

/*****************************************************************************
 函 数 名  : CalcSpeed
 功能描述  : 转速计算
 输入参数  : 无
 输出参数  : void
*****************************************************************************/
static inline void calcSpeedTime(void) {
    if (++HoldParm.SpeedTime_Cnt > 6) {
        SpeedSetTime(HoldParm.SpeedTime_Sum);
        HoldParm.SpeedTime_Cnt = 0;
        HoldParm.SpeedTime_Sum = 0;
    } else {
        HoldParm.SpeedTime_Sum += HoldParm.ZeroCrossTime;
    }
}

/*****************************************************************************
 函 数 名  : EnterRunInit
 功能描述  : 进入Run的初始化
 输入参数  : 无
 输出参数  : void
*****************************************************************************/
void EnterRunInit(void) {
}

/*****************************************************************************
 函 数 名  : StartupDrag
 功能描述  : 启动，边强拖，边检测
 输入参数  : 无
 输出参数  : void
*****************************************************************************/
FAST_FUNC bool StartupDrag(void) {
    static uint16_t ADC_CNT = 0;
    static uint16_t bemfMax = 0;
    if (StartupCheckZeroCross(HoldParm.RotorDirection, HoldParm.Phase, ADCSample.BemfPhase, ADCSample.OnPhase)) {
        if (++HoldParm.Phase > 5) {
            HoldParm.Phase = 0;
        }
        PWMSwitchPhase();
        ADC_CNT = 0;
        bemfMax = 0;
        if (StartupIsFinish()) {
            StartupReset();
            return true;
        }

    } else if (++ADC_CNT >= HoldParm.DragCnt) {
        ADC_CNT = 0;

        StartupReset();

        // 检测是否缺相
        if (bemfMax < PHASE_LOSS_THRESHOLD) {
            ErrorCode.bit.PhaseLoss = 1;
            MotorStop();
            bemfMax = 0;
            return false;
        }

        bemfMax = 0;

        // TODO 考虑后续优化升压升频方式
        HoldParm.DragCnt -= ((HoldParm.DragCnt >> 6) + 1);
        HoldParm.PWMDutyCycle += 1;
        UP16LIMIT(HoldParm.PWMDutyCycle, DRAG_PWM_DUTY_MAX, DRAG_PWM_DUTY_MIN);
        UP16LIMIT(HoldParm.DragCnt, DRAG_CNT_MAX, DRAG_CNT_MIN);
        MotorSetDuty(HoldParm.PWMDutyCycle);
        if (++HoldParm.Phase > 5) {
            HoldParm.Phase = 0;
        }
        PWMSwitchPhase();
    } else {
        if (bemfMax < ADCSample.BemfPhase) {
            bemfMax = ADCSample.BemfPhase;
        }
    }

    return false;
}

/*****************************************************************************
 函 数 名  : MotorFault
 功能描述  : 电机等待重启状态，电流保护，CMP保护，堵转保护。
 输入参数  : 无
 输出参数  : void
*****************************************************************************/
// static void MotorFault(void) {
// }

/*****************************************************************************
 函 数 名  : MotorControl
 功能描述  : 电机控制
 输入参数  : 无
 输出参数  : void
*****************************************************************************/
FAST_FUNC void MotorControl(void) {
    AdcSample(gMultiSamplingPointNow == gMultiSamplingCnt - 1);
    if (gMotorIsNeedRun == false) {
        if (mcState != mcIdle) {
            mcState = mcStop;
        }
    } else {
        PWMChangeDuty(gDuty);
    }

    switch (mcState) {
    case mcIdle:
        idle();
        break;
    case mcCharge:
        charge();
        break;
    case mcAhead:
        ahead();
        break;
    case mcAlign:
        alignTask();
        break;
    case mcDrag:
        drag();
        break;
    case mcRun:
        run();
        break;
    case mcdec:
        run();
        brake();
        break;
    case mcStop:
        PWMSetAllOff();
        DrvSystickCmd(false);
        gDuty = 0;
        mcState = mcIdle;
        break;
    default:
        break;
    }

    AdcSampleChannelCharge();
}

// MotorStart 启动电机
void MotorStart(void) {
    if (gMotorIsNeedRun == true) {
        return;
    }
    gMotorIsNeedRun = true;
    gMotorTriggered = true;
}

// MotorStop 停止电机
void MotorStop(void) {
    if (gMotorIsNeedRun == false) {
        return;
    }
    gMotorIsNeedRun = false;
    gMotorTriggered = true;
}

// MotorSetDuty 设置占空比
void MotorSetDuty(uint16_t duty) {
    if (duty == gDuty) {
        return;
    }

    if (duty > PWM_MAX_DUTY) {
        duty = PWM_MAX_DUTY;
    }

    if (duty < PWM_MIN_DUTY) {
        duty = PWM_MIN_DUTY;
    }

    if (gDuty > duty) {
        multiSamplingCalc(duty);
        gDuty = duty;
    } else {
        gDuty = duty;
        multiSamplingCalc(duty);
    }
}

static inline void multiSamplingCalc(uint16_t duty) {
    uint8_t i = 0;
    int32_t temp = duty - (SAMPLING_INTERVAL_CNT * 3 + SAMPLING_TIME_CNT + AVOID_SWITCH_CNT);

    if (temp > 0) {
        gMultiSamplingPoint[i++] = temp;
    }

    temp = duty - (SAMPLING_INTERVAL_CNT * 2 + SAMPLING_TIME_CNT + AVOID_SWITCH_CNT);
    if (temp > 0) {
        gMultiSamplingPoint[i++] = temp;
    }

    temp = duty - (SAMPLING_INTERVAL_CNT * 1 + SAMPLING_TIME_CNT + AVOID_SWITCH_CNT);
    if (temp > 0) {
        gMultiSamplingPoint[i++] = temp;
    }

    gMultiSamplingPoint[i++] = duty - SAMPLING_TIME_CNT;

    gMultiSamplingCnt = i;

    for (; i < 4; i++) {
        gMultiSamplingPoint[i] = 0;
    }
}
