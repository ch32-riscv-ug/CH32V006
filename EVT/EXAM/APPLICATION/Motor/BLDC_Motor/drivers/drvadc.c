/********************************** (C) COPYRIGHT  *******************************
 * File Name          : drvadc.c
 * Author             : WCH
 * Version            : V1.0.1
 * Date               : 2026/11/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "drvadc.h"

// DrvAdcInit: ADC initialization
static void irqinit(void);
static void adcRegsInit(void);
static void dmaInit(void);
static void dmaRestart(void);
static void phaseSequenceInit(void);

// Single-channel buffer for the regular group (VBUS) moved cyclically by DMA1_Ch1
static volatile uint16_t gDmaRegular[1] __attribute__((aligned(4)));
static volatile bool gRegularHasData = false;
// TE stays latched until the recovery path restarts DMA; the error is counted only once.
static volatile bool gDmaErrorCounted = false;
volatile DrvAdcDiag_T gDrvAdcDiag = {0};

void DrvAdcInit(void) {
    // Configuration register structure definition
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    OPA_InitTypeDef OPA_InitStructure = {0};

    // Clock enable
    RCC_PB2PeriphClockCmd(RCC_PB2Periph_GPIOA | RCC_PB2Periph_GPIOC | RCC_PB2Periph_GPIOD | RCC_PB2Periph_AFIO | RCC_PB2Periph_ADC1, ENABLE);

    // ADC clock prescaler configuration
    RCC_ADCCLKConfig(RCC_PCLK2_Div2);

    // ADC pin configuration
    GPIO_StructInit(&GPIO_InitStructure);
    // The three-phase voltage sampling pins are all on GPIOD
    GPIO_InitStructure.GPIO_Pin = DRVADC_U_PIN | DRVADC_V_PIN | DRVADC_W_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(DRVADC_U_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = DRVADC_VBUS_PIN;
    GPIO_Init(DRVADC_VBUS_PORT, &GPIO_InitStructure);

    // OPA unlock
    OPA_Unlock(); // TODO: lock again later
    OPA_CMP_Unlock();

    // OPA and CMP configuration: the OPA output only feeds CMP2 for hardware over-current brake, not routed to an IO
    GPIO_InitStructure.GPIO_Pin = DRVADC_OPA_P_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(DRVADC_OPA_P_PORT, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = DRVADC_OPA_N_PIN;
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
    OPA->CTLR2 |= 0x0B010000; // Enable CMP2, select T1 as break source, enable CMP digital filter
    OPA_Cmd(ENABLE);

    OPA_Lock();

    adcRegsInit();
    dmaInit();

    ADC_Cmd(ADC1, ENABLE);
    ADC_ClearITPendingBit(ADC1, ADC_IT_JEOC);
    irqinit();
    ADC_ITConfig(ADC1, ADC_IT_JEOC, ENABLE);
}

// Reset and configure the ADC registers; shared by power-on and pulse-detection recovery,
// ensuring the regular group starts from the first channel.
// OPA/CMP2 are independent peripherals, so an ADC reset does not affect the hardware over-current protection.
static void adcRegsInit(void) {
    ADC_InitTypeDef ADC_InitStructure = {0};

    // ADC configuration reset
    ADC_DeInit(ADC1);

    ADC_StructInit(&ADC_InitStructure);                // Initialization of the structure used for AD initialization
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent; // Independent mode
    ADC_InitStructure.ADC_ScanConvMode = ENABLE;       // Scan enable
    // On this chip, with the regular group EXTSEL=SWSTART a single software trigger produces only one sequence and CONT does not self-continue;
    // the regular group is attached to the TIM1 update event (TRGO) instead, TIM1 runs continuously and triggers one single-channel conversion per PWM period.
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_TRGO;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right; // Data is right aligned
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure); // Configuration register instance

    // The regular group is triggered periodically by TIM1_TRGO and has only one channel, the bus voltage VBUS;
    // this board has no externally RC-filtered IBUS channel, so the average current is obtained by IIR filtering the injected-group OPA samples in the main loop.
    ADC_RegularChannelConfig(ADC1, DRVADC_VBUS_CHANNEL, 1, ADC_SampleTime_CyclesMode1);
    ADC_ExternalTrigConvCmd(ADC1, ENABLE);
    phaseSequenceInit();
    ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_T3_CC1);
    ADC_ExternalTrigInjectedConvCmd(ADC1, ENABLE);
    ADC_AutoInjectedConvCmd(ADC1, DISABLE);
}

static void phaseSequenceInit(void) {
    // The injected sequence is fixed to the four channels U/V/W/OPA, with rank1..4 mapped to JSQ1..4 in order and the results stored in IDATAR1..4;
    // the shared control.c reads the three-phase voltages and the OPA current according to this fixed layout
    ADC_InjectedSequencerLengthConfig(ADC1, 4);
    ADC_InjectedChannelConfig(ADC1, DRVADC_U_CHANNEL, 1, ADC_SampleTime_CyclesMode0);
    ADC_InjectedChannelConfig(ADC1, DRVADC_V_CHANNEL, 2, ADC_SampleTime_CyclesMode0);
    ADC_InjectedChannelConfig(ADC1, DRVADC_W_CHANNEL, 3, ADC_SampleTime_CyclesMode0);
    ADC_InjectedChannelConfig(ADC1, DRVADC_OPA_CHANNEL, 4, ADC_SampleTime_CyclesMode0);
}

static void dmaInit(void) {
    DMA_InitTypeDef dma = {0};

    RCC_HBPeriphClockCmd(RCC_HBPeriph_DMA1, ENABLE);
    DMA_DeInit(DMA1_Channel1);
    dma.DMA_PeripheralBaseAddr = (uint32_t)(uintptr_t)&ADC1->RDATAR;
    dma.DMA_MemoryBaseAddr = (uint32_t)(uintptr_t)&gDmaRegular;
    dma.DMA_DIR = DMA_DIR_PeripheralSRC;
    dma.DMA_BufferSize = 1;
    dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    dma.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    dma.DMA_Mode = DMA_Mode_Circular;
    dma.DMA_Priority = DMA_Priority_VeryHigh;
    dma.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel1, &dma);
    gRegularHasData = false;
    gDmaErrorCounted = false;
    DMA_Cmd(DMA1_Channel1, ENABLE);
    ADC_DMACmd(ADC1, ENABLE);
}

static void dmaRestart(void) {
    DMA_Cmd(DMA1_Channel1, DISABLE);
    DMA_ClearFlag(DMA1_FLAG_GL1);
    DMA_SetCurrDataCounter(DMA1_Channel1, 1);
    gRegularHasData = false;
    gDmaErrorCounted = false;
    DMA_Cmd(DMA1_Channel1, ENABLE);
}

bool DrvAdcGetRegularSample(DrvAdcRegularSample_T *sample) {
    uint32_t flags = DMA1->INTFR;
    if ((flags & DMA1_FLAG_TE1) != 0 || (DMA1_Channel1->CFGR & DMA_CFGR1_EN) == 0 ||
        (gRegularHasData == false && (flags & DMA1_FLAG_TC1) == 0)) {
        return false;
    }
    sample->current = 0;
    sample->voltage = gDmaRegular[0];
    return true;
}

bool DrvAdcReadRegular(DrvAdcRegularSample_T *sample) {
    uint32_t flags = DMA1->INTFR;
    if ((flags & DMA1_FLAG_TE1) != 0) {
        if (gDmaErrorCounted == false) {
            gDmaErrorCounted = true;
            gDrvAdcDiag.dmaErrorCount++;
        }
        return false;
    }
    if ((flags & DMA1_FLAG_TC1) != 0) {
        gRegularHasData = true;
        DMA_ClearFlag(DMA1_FLAG_HT1 | DMA1_FLAG_TC1);
        gDrvAdcDiag.frameCount++;
    }
    if (DrvAdcGetRegularSample(sample) == false) {
        gDrvAdcDiag.missedCount++;
        return false;
    }
    gDrvAdcDiag.voltageRaw = sample->voltage;
    return true;
}

void DrvAdcPauseRegular(void) {
    ADC_ExternalTrigConvCmd(ADC1, DISABLE);
    ADC_AutoInjectedConvCmd(ADC1, DISABLE);
    ADC_Cmd(ADC1, DISABLE);
    ADC1->CTLR2 &= ~ADC_CONT;
    ADC_DMACmd(ADC1, DISABLE);
    DMA_Cmd(DMA1_Channel1, DISABLE);
    gRegularHasData = false;
    ADC_Cmd(ADC1, ENABLE);
}

void DrvAdcResumeRegular(void) {
    // Turning ADON off and on again does not guarantee that the regular group sequence pointer returns to the first channel; after pulse detection interrupts a scan,
    // it may continue from the middle of the sequence. Therefore the ADC is fully reset following the power-on procedure.
    ADC_Cmd(ADC1, DISABLE);
    adcRegsInit();
    dmaRestart();
    ADC_DMACmd(ADC1, ENABLE);
    ADC_Cmd(ADC1, ENABLE);
    ADC_ClearITPendingBit(ADC1, ADC_IT_JEOC);
    ADC_ITConfig(ADC1, ADC_IT_JEOC, ENABLE);
}

static void irqinit(void) {
    NVIC_InitTypeDef NVIC_InitStructure = {0};
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1); // Interrupt priority grouping configuration

    // ADC injected interrupt configuration: preemption priority 0. Under the shared library's single ADC direct-PWM / sampling point update mechanism,
    // the run-time PWM duty cycle and the TIM3 sampling plan are updated directly by the ADC JEOC interrupt and take effect immediately, while TIM1_UP only
    // keeps the time accounting and publishes the safety time frame; therefore the ADC must be able to preempt TIM1_UP so that time frame accounting does not delay the PWM / sampling point loading.
    NVIC_InitStructure.NVIC_IRQChannel = ADC_IRQn;            // Interrupt channel ADC_IRQn
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // Preemption priority 0, can preempt TIM1_UP
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;        // Sub priority 1
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           // Interrupt enable
    NVIC_Init(&NVIC_InitStructure);                           // Configuration register instance
}

// DrvAdcGetVbusValue: get the bus voltage
uint16_t DrvAdcGetVbusValue(void) {
    DrvAdcRegularSample_T sample;
    return DrvAdcGetRegularSample(&sample) ? sample.voltage : gDrvAdcDiag.voltageRaw;
}
