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
void Waveform_Display (void);//Waveform display variable setup

/* Private variables --------------------------------------------------------*/
/* Variables ----------------------------------------------------------------*/
/******************************************************************************
* Function Name  : Interrupt_Configuration
* Description    : Interrupt priority configuration
* Input          : None
* Output         : None
* Return         : None
******************************************************************************/
void Interrupt_Configuration(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);             //Interrupt priority group configuration

    //ADC injection interrupt configuration
    NVIC_InitStructure.NVIC_IRQChannel = ADC_IRQn;              //Interrupt channel: ADC_IRQn
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;   //Preemption priority: 0
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;          //Sub priority: 1
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;             //Interrupt enable
    NVIC_Init(&NVIC_InitStructure);                             //Write configuration into registers

    //Timer1 interrupt configuration
    NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;          //Interrupt channel: TIM1_UP_IRQn
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;   //Preemption priority: 0
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;          //Sub priority: 2
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;             //Interrupt enable
    NVIC_Init(&NVIC_InitStructure);                             //Write configuration into registers

    //Brake interrupt configuration
    NVIC_InitStructure.NVIC_IRQChannel = TIM1_BRK_IRQn;         //Interrupt channel: TIM1_BRK_IRQn
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;   //Preemption priority: 0
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;          //Sub priority: 1
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;             //Interrupt enable
    NVIC_Init(&NVIC_InitStructure);                             //Write configuration into registers
    TIM_ClearITPendingBit(TIM1, TIM_IT_Break);                  //Clear the brake interrupt pending flag
    TIM_ITConfig(TIM1, TIM_IT_Break,ENABLE);                    //Enable the brake interrupt

    //SysTick interrupt configuration
    NVIC_InitStructure.NVIC_IRQChannel = SysTicK_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;   //Preemption priority: 1
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;          //Sub priority: 2
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/******************************************************************************
* Function Name  : TIM1_UP_IRQHandler
* Description    : Timer1 update interrupt, waveform data output processing
* Input          : None
* Output         : None
* Return         : None
******************************************************************************/
void TIM1_UP_IRQHandler(void)
{
    if( RunningStatus_M ==PRESTART)//Motor start processing
    {
        TIM1->CH4CVR = 48;//ADC trigger instant
        ADC_Start(ENABLE);//ADC start
        RunningStatus_M=DIRCHECK;
        if(RunningStatus_M==DIRCHECK)
        {
           //Initialize the LPF parameters of the observed d axis current
            MStruc_M.IdLPF1stH.Input_Coef  = IDQLPF_HIGH_INPUT_COEF;
            MStruc_M.IdLPF1stH.Output_Coef = IDQLPF_HIGH_OUTPUT_COEF;

           //Initialize the LPF parameters of the observed q axis current
            MStruc_M.IqLPF1stH.Input_Coef  = IDQLPF_HIGH_INPUT_COEF;
            MStruc_M.IqLPF1stH.Output_Coef = IDQLPF_HIGH_OUTPUT_COEF;

            //Increase the PI gain of the current loop
            Mpid_M.dCurr.Kp=Mpid_M.dCurr.Kp<<FLYSTART_CURRLOOP_GAIN_M;
            Mpid_M.dCurr.Ki=Mpid_M.dCurr.Ki<<FLYSTART_CURRLOOP_GAIN_M;
            Mpid_M.qCurr.Kp=Mpid_M.qCurr.Kp<<FLYSTART_CURRLOOP_GAIN_M;
            Mpid_M.qCurr.Ki=Mpid_M.qCurr.Ki<<FLYSTART_CURRLOOP_GAIN_M;
        }
    }
    if((Flystart_M.Process_mark==TRUE)&&(RunningStatus_M == DIRCHECK))
     {
         Flystart_Switch(&Flystart_M,&Ramp_M);
     }

    DCBUS_Volt_Cal(&ADC_M,ADC1);//Bus voltage calculation

    ADC_SoftwareStartConvCmd(ADC1, ENABLE);//Bus voltage sampling trigger

    // Waveform_Display();
    TIM1->INTFR = (uint16_t)~TIM_FLAG_Update;//Clear the update event interrupt flag
}

/******************************************************************************
* Function Name  : TIM1_BRK_IRQHandler
* Description    : Brake interrupt, set the fault
* Input          : None
* Output         : None
* Return         : None
******************************************************************************/
void TIM1_BRK_IRQHandler(void)
{
    Protection_SetFault(DC_OVER_CURR_HARD_M);
    TIM_ClearITPendingBit(TIM1, TIM_IT_Break);
}

/*******************************************************************************
* Function Name  : NMI_Handler
* Description    : This function handles NMI exception.
* Input          : None
* Return         : None
*******************************************************************************/
void NMI_Handler(void)
{

}

/*******************************************************************************
* Function Name  : HardFault_Handler
* Description    : This function handles Hard Fault exception.
* Input          : None
* Return         : None
*******************************************************************************/
void HardFault_Handler(void)
{
  while (1)
  {
      //NVIC_SystemReset();
  }
}
