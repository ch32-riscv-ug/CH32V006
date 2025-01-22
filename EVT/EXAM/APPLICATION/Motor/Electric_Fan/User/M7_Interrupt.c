/********************************** (C) COPYRIGHT *******************************
 * File Name          : M7_Interrupt.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2024/11/04
 * Description        : Interrupt configuration

*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
/* Includes -----------------------------------------------------------------*/
#include "M0_Control_Library.h"

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private functions --------------------------------------------------------*/
void TIM1_UP_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM1_BRK_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void Waveform_Display (void);

/* Private variables --------------------------------------------------------*/
/* Variables ----------------------------------------------------------------*/
/*********************************************************************
 * @fn      Interrupt_Configuration
 *
 * @brief   Interrupt priority configuration
 *
 * @return  none
 */
void Interrupt_Configuration(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);             //�ж����ȼ���������

    //ADCע���ж�����
    NVIC_InitStructure.NVIC_IRQChannel = ADC_IRQn;              //�ж�ͨ��ADC_IRQn
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;   //��ռ���ȼ�0
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;          //�����ȼ�1
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;             //�ж�ʹ��
    NVIC_Init(&NVIC_InitStructure);                             //���üĴ���ʵ��

    //Timer1�ж�����
    NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;          //�ж�ͨ��TIM1_UP_IRQn
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;   //��ռ���ȼ�0
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;          //�����ȼ�2
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;             //�ж�ʹ��
    NVIC_Init(&NVIC_InitStructure);                             //���üĴ���ʵ��

    //ɲ���ж�����
    NVIC_InitStructure.NVIC_IRQChannel = TIM1_BRK_IRQn;         //�ж�ͨ��TIM1_BRK_IRQn
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;   //��ռ���ȼ�0
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;          //�����ȼ�1
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;             //�ж�ʹ��
    NVIC_Init(&NVIC_InitStructure);                             //���üĴ���ʵ��
    TIM_ClearITPendingBit(TIM1, TIM_IT_Break);                  //���ɲ���жϱ�־���
    TIM_ITConfig(TIM1, TIM_IT_Break,ENABLE);                    //ʹ��ɲ���ж�

    //SysTicK�ж�����
    NVIC_InitStructure.NVIC_IRQChannel = SysTicK_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;   //��ռ���ȼ�Ϊ1
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;          //�����ȼ�Ϊ2
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/*********************************************************************
 * @fn      TIM1_UP_IRQHandler
 *
 * @brief   Timer 1 underflow interrupt
 *
 * @return  none
 */
__attribute__((section(".highcode")))
void TIM1_UP_IRQHandler(void)
{
    if( RunningStatus_M == PRESTART)//�����������
    {
        TIM1->CH4CVR = 48;//ADC����ʱ��
        ADC_Start(ENABLE);//ADC����
        RunningStatus_M=DIRCHECK;
    }

    if((Flystart_M.Process_mark==TRUE)&&(RunningStatus_M == DIRCHECK))
     {
         Flystart_Switch(&Flystart_M,&SpeedRamp_M);
     }

    ADC_SoftwareStartConvCmd(ADC1, ENABLE);//ĸ�ߵ�ѹ��������
    DCBUS_Volt_Cal(&ADC_M,ADC1);//ĸ�ߵ�ѹ����

//    Waveform_Display();//����ʾ�����۲�
    TIM1->INTFR = (uint16_t)~TIM_FLAG_Update;//��������¼��жϱ�־
}
/*********************************************************************
 * @fn      TIM1_BRK_IRQHandler
 *
 * @brief   Brake interrupt
 *
 * @return  none
 */
void TIM1_BRK_IRQHandler(void)
{
    Protection_SetFault(DC_OVER_CURR_HARD_M);
    TIM_ClearITPendingBit(TIM1, TIM_IT_Break);
}
/*********************************************************************
 * @fn      NMI_Handler
 *
 * @brief   This function handles NMI exception.
 *
 * @return  none
 */
void NMI_Handler(void)
{
    while (1)
    {
    }
}
/*********************************************************************
 * @fn      HardFault_Handler
 *
 * @brief   This function handles Hard Fault exception.
 *
 * @return  none
 */
void HardFault_Handler(void)
{
    NVIC_SystemReset();
    while (1)
    {
    }
}
