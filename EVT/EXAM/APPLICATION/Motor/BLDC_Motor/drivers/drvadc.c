/********************************** (C) COPYRIGHT  *******************************
 * File Name          : drvadc.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2026/08/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "drivers.h"

// DrvAdcInit ADC初始化
static void irqinit(void);

void DrvAdcInit(void) {
    // 配置寄存器结构体定义
    ADC_InitTypeDef ADC_InitStructure = {0};
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    OPA_InitTypeDef OPA_InitStructure = {0};

    // 时钟使能
    RCC_PB2PeriphClockCmd(RCC_PB2Periph_GPIOA | RCC_PB2Periph_GPIOC | RCC_PB2Periph_GPIOD | RCC_PB2Periph_AFIO | RCC_PB2Periph_ADC1, ENABLE);

    // ADC时钟预分频配置
    RCC_ADCCLKConfig(RCC_PCLK2_Div2);

    // ADC引脚配置
    GPIO_StructInit(&GPIO_InitStructure);
    //
    GPIO_InitStructure.GPIO_Pin = DRVADC_U_PIN | DRVADC_V_PIN | DRVADC_W_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(DRVADC_U_PORT, &GPIO_InitStructure);
    // GPIO_Init(DRVADC_V_PORT, &GPIO_InitStructure);
    // GPIO_Init(DRVADC_W_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = DRVADC_VBUS_PIN;
    GPIO_Init(DRVADC_VBUS_PORT, &GPIO_InitStructure);

    // OPA解锁---
    OPA_Unlock(); //--补充：后续需要上锁--
    OPA_CMP_Unlock();

    // OPA和CMP配置
    GPIO_InitStructure.GPIO_Pin = DRVADC_OPA_P_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(DRVADC_OPA_P_PORT, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = DRVADC_OPA_N_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(DRVADC_OPA_N_PORT, &GPIO_InitStructure);

    OPA_InitStructure.PSEL = CHP1;
    OPA_InitStructure.NSEL = CHN_PGA_8xIN;
    OPA_InitStructure.Mode = OUT_CMP2_ONLY;
    OPA_InitStructure.FB = FB_ON;

    OPA_InitStructure.PGA_VBEN = PGA_VBEN_ON;
    OPA_InitStructure.PGA_VBSEL = PGA_VBSEL_VDD_DIV2;
    OPA_InitStructure.PGADIF = PGADIF_ON;
    OPA_InitStructure.VBCMPSEL = VBCMPSEL_Mode_0;

    OPA_InitStructure.OPA_HS = HS_ON;
    OPA_Init(&OPA_InitStructure);

    OPA_CMP_Cmd(CMP2, ENABLE);
    OPA->CTLR2 |= 0x0B010000; // CMP2使能，T1刹车源选择,CMP数字滤波使能
    OPA_Cmd(ENABLE);

    OPA_Lock();

    // ADC配置复位
    ADC_DeInit(ADC1);

    ADC_StructInit(&ADC_InitStructure);                                 // AD初始化用结构体的初始化
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;                  // 独立模式
    ADC_InitStructure.ADC_ScanConvMode = ENABLE;                        // 扫描使能
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;                 // 连续转换禁止
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None; // 采用软件触发转换
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;              // 数据采用右对齐模式
    ADC_InitStructure.ADC_NbrOfChannel = 1;                             // 转换通道个数为1
    ADC_Init(ADC1, &ADC_InitStructure);                                 // 配置寄存器实体

    ADC_InjectedSequencerLengthConfig(ADC1, 4); /*00-11：1-4 个转换。*/
    ADC_InjectedChannelConfig(ADC1, DRVADC_OPA_CHANNEL, 1, ADC_SampleTime_CyclesMode0);
    ADC_InjectedChannelConfig(ADC1, DRVADC_U_PIN, 2, ADC_SampleTime_CyclesMode0);
    ADC_InjectedChannelConfig(ADC1, DRVADC_W_PIN, 3, ADC_SampleTime_CyclesMode0);
    ADC_InjectedChannelConfig(ADC1, DRVADC_VBUS_CHANNEL, 4, ADC_SampleTime_CyclesMode0);

    // 规则通道配置
    // ADC_RegularChannelConfig(ADC1, DRVADC_VBUS_CHANNEL, 1, ADC_SampleTime_CyclesMode0);

    ADC_Cmd(ADC1, ENABLE); // ADC使能

    irqinit();

    // 使能外部事件启动注入转换
    ADC_ExternalTrigInjectedConvCmd(ADC1, ENABLE);
    // ADC的外部触发源为T1的CC4
    ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_T1_CC4);
    // 清除ADC转换完成标志，下次中断做准备
    ADC_ClearITPendingBit(ADC1, ADC_IT_JEOC);

    ADC_ITConfig(ADC1, ADC_IT_JEOC, ENABLE);
}

static void irqinit(void) {
    NVIC_InitTypeDef NVIC_InitStructure = {0};
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1); // 中断优先级分组配置

    // ADC注入中断配置
    NVIC_InitStructure.NVIC_IRQChannel = ADC_IRQn;            // 中断通道ADC_IRQn
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // 抢占优先级0
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;        // 从优先级1
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           // 中断使能
    NVIC_Init(&NVIC_InitStructure);                           // 配置寄存器实体
}

// DrvAdcGetValue ADC获取值
// channel: DRVADC_OPA, DRVADC_VBUS, DRVADC_BEMF
uint16_t DrvAdcGetValue(DrvAdcChannel channel) {
    switch (channel) {
    case DRVADC_OPA:
        return ADC1->IDATAR1;
    case DRVADC_VBUS:
        return ADC1->IDATAR2;
    case DRVADC_BEMF:
        return ADC1->IDATAR3;
    default:
        return 0;
    }
}

// DrvAdcSetBemfChannel 设置BEMF ADC注入通道
// channel: DRVADC_U, DRVADC_V, DRVADC_W
void DrvAdcSetBemfChannel(DrvAdcChannel channel) {
    uint8_t adcChannel = 0;
    switch (channel) {
    case DRVADC_U:
        adcChannel = DRVADC_U_CHANNEL;
        break;
    case DRVADC_V:
        adcChannel = DRVADC_V_CHANNEL;
        break;
    case DRVADC_W:
        adcChannel = DRVADC_W_CHANNEL;
        break;
    default:
        break;
    }
    ADC_InjectedChannelConfig(ADC1, adcChannel, 3, ADC_SampleTime_CyclesMode0);
}

// DrvAdcGetVbusValue 获取母线电压
uint16_t DrvAdcGetVbusValue(void) {
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC))
        ;
    ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
    return ADC1->RDATAR;
}
