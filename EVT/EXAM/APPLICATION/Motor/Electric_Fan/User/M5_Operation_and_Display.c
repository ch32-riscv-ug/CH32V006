/* Includes -----------------------------------------------------------------*/
#include "M0_Control_Library.h"
/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private Functions --------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Variables ----------------------------------------------------------------*/
static volatile uint16_t Timewait = 0;//time used for delay or wait, timebase is the interrupt of processing count

Rampctr_Type   Ramp_M;                  //ramp control structure
Operation_Type      Operation_M;        //system operation structure
Powercontrol_Type   Powerctr_M;         //power on/off control structure
Key_Manager_Type    Key_M;              //KEY manager structure
/******************************************************************************
* Function Name  : Motor_Operation_Process
* Description    : Motor operation processing
* Input          : None
* Output         : None
* Return         : None
******************************************************************************/
void Motor_Operation_Process(void)
{
    //motor running state machine processing
    switch (RunningStatus_M)
    {
        case IDLE:
            if (Powerctr_M.Poweron == TRUE)
            {
                Ramp_Set(&Ramp_M);    //speed setting
            }
//            RunningStatus_M = INIT;
            break;

        case INIT:
            Motor_Start_Init_M();       //motor startup initialization
            RunningStatus_M = PRESTART;    //switch motor running state to pre-start
            break;

        case PRESTART://wait for timer underflow interrupt to start officially
            break;

        case POSITION://positioning, switch to start state during positioning
            break;

        case START://start, switch to run state during startup
            break;

        case RUN://run

            break;

        case STOP://stop
            TIM_CtrlPWMOutputs(TIM1, DISABLE);  //PWM output disable
            ADC_Start(DISABLE);                 //ADC conversion disable
            RunningStatus_M = WAIT;             //motor running state machine switches to WAIT state
            Time_Delay_Set(Operation_M.StopwaitSet,&Operation_M.StopwaitCnt);//stop wait delay setting
            Operation_M.Checkmode = 0;
		   break;

        case WAIT://wait
            //stop wait time elapsed, set IDLE state
            if (Time_Delay_Elapse(Operation_M.StopwaitCnt) == TRUE)
                RunningStatus_M=IDLE;

            break;
        case FAULT://fault
            TIM_CtrlPWMOutputs(TIM1, DISABLE);  //PWM output disable
            ADC_Start(DISABLE);                 //ADC conversion disable
            break;
        default:
            break;
    }
}
/******************************************************************************
* Function Name  : Poweroperation_Initial
* Description    : Power on/off parameter and status initialization
* Input          : Pointer to the power on/off control structure
* Output         : None
* Return         : None
*****************************************************************************/
void Poweroperation_Initial(Powercontrol_Type *PowerOper)
{
    PowerOper->Poweron_CntThresh = (uint16_t)(POWERVERY_FREQ*POWERON_TIME);
    PowerOper->Poweron = FALSE;
    PowerOper->Poweron_Cnt = 0;
}

/******************************************************************************
* Function Name  : Operation_Parameter_Define
* Description    : Operation parameter definition
* Input          : Pointer to the operation control structure
* Output         : None
* Return         : None
*****************************************************************************/
void Operation_Parameter_Define(Operation_Type *Operation)
{
    Operation->StopwaitSet = (uint16_t)(STOP_WAITING_TIME * OPERATION_CAL_FREQ);
    Operation->FaultHoldingSet = (uint16_t)(FAULT_HOLDING_TIME * OPERATION_CAL_FREQ);
    Operation->Startmark = FALSE;
//    Operation->Restartmark = FALSE;
    Operation->Checkmode = 0;
}

/******************************************************************************
* Function Name  : Operation_Status_Init
* Description    : Operation status initialization
* Input          : Pointer to the operation control structure
* Output         : None
* Return         : None
*****************************************************************************/
void Operation_Status_Init(Operation_Type *Operation)
{
    Operation->StopwaitCnt = 0;
    Operation->FaultHoldingCnt = 0;
}

/******************************************************************************
* Function Name  : rampctr_Parameter_Define
* Description    : Ramp control parameter definition
* Input          : Pointer to the ramp speed control structure
* Output         : None
* Return         : None
*****************************************************************************/
void rampctr_Parameter_Define(Rampctr_Type *Ramp_M)
{
    Ramp_M->Direction = STARTUP_DIRECTION;
    Ramp_M->Point_adncnt = POINT_DEFAULT;

    Ramp_M->AccStep = _IQ(1.0)/(SPEED_LOOP_CAL_FREQ*ACCE_TIME_M);
    Ramp_M->DecStep = _IQ(1.0)/(SPEED_LOOP_CAL_FREQ*DECE_TIME_M);
}

/******************************************************************************
* Function Name  : rampctr_Initial
* Description    : Ramp control status initialization
* Input          : Pointer to the ramp control structure, loop
* Output         : None
* Return         : None
*****************************************************************************/
void rampctr_Initial(Rampctr_Type *Ramp_M)
{
    Ramp_Set(Ramp_M);

#if(CLOSE_LOOP==SPEED_LOOP)
    Ramp_M->Aftramp = SPEEDPOINT1*Ramp_M->Direction;
#elif (CLOSE_LOOP==POWER_LOOP)
    Ramp_M->Aftramp =  POWERPOINT1;
#endif
    Ramp_M->Startfinish =  FALSE;
    Ramp_M->Decemark =FALSE;

    System_Status_Global = SPEED_CONTROL_M;
}

/******************************************************************************
* Function Name  : Ramp_Set
* Description    : Ramp setting
* Input          : Pointer to the acceleration/deceleration control structure
* Output         : None
* Return         : None
*****************************************************************************/
void Ramp_Set(Rampctr_Type *Ramp_M)
{
#if     (CLOSE_LOOP==SPEED_LOOP)
    switch(Ramp_M->Point_adncnt)
    {
        case 1://Speed gear 1
           Ramp_M->Preramp =  SPEEDPOINT1*Ramp_M->Direction;
           break;
       case 2://Speed gear 2
           Ramp_M->Preramp =  SPEEDPOINT2*Ramp_M->Direction;
           break;
       case 3://Speed gear 3
           Ramp_M->Preramp =  SPEEDPOINT3*Ramp_M->Direction;
           break;
       default:
           break;
    }
#elif   (CLOSE_LOOP==POWER_LOOP)
    switch(Ramp_M->Point_adncnt)
    {
        case 1://Speed gear 1
           Ramp_M->Preramp =  POWERPOINT1;
           break;
       case 2://Speed gear 2
           Ramp_M->Preramp =  POWERPOINT2;
           break;
       case 3://Speed gear 3
           Ramp_M->Preramp =  POWERPOINT3;
           break;
       default:
           break;
    }
#endif
}
/******************************************************************************
* Function Name  : Speed_Ramp_Process
* Description    : Speed ramp processing, executed in the loop, the acceleration/deceleration calculation frequency must be the same as the interrupt frequency;
* Input          : Pointer to the speed ramp structure
* Output         : Speed after the ramp processing
* Return         : None
******************************************************************************/
void Speed_Ramp_Process(Rampctr_Type *Speed_Ramp)
{
    _iq24 Speed_Ref_Output;
    Speed_Ref_Output = Speed_Ramp->Aftramp;

    //Both the new speed and the current speed are >= 0
    if((Speed_Ref_Output >=_IQ24(0.0)) && (Speed_Ramp->Preramp >=_IQ24(0.0)))
    {
        if(Speed_Ref_Output < Speed_Ramp->Preramp)//Acceleration processing
        {
            Speed_Ref_Output += Speed_Ramp->AccStep;
            if(Speed_Ref_Output > Speed_Ramp->Preramp)//Final value processing
            {
               Speed_Ref_Output = Speed_Ramp->Preramp;
            }
            else;
        }
        else if(Speed_Ref_Output > Speed_Ramp->Preramp)//Deceleration processing
        {
            Speed_Ref_Output -= Speed_Ramp->DecStep;
            if(Speed_Ref_Output < Speed_Ramp->Preramp)//Final value processing
            {
               Speed_Ref_Output = Speed_Ramp->Preramp;
            }
            else;
        }
        else;
    }

    //New speed < 0, current speed > 0
    else if((Speed_Ref_Output >=_IQ24(0.0)) && (Speed_Ramp->Preramp < _IQ24(0.0)))
    {
        Speed_Ref_Output -= Speed_Ramp->DecStep;
    }

    //Both the new speed and the current speed are < 0
    else if((Speed_Ref_Output < _IQ24(0.0)) && (Speed_Ramp->Preramp < _IQ24(0.0)))
     {
         if(Speed_Ref_Output < Speed_Ramp->Preramp)//Deceleration processing
         {
            Speed_Ref_Output += Speed_Ramp->DecStep;
            if(Speed_Ref_Output > Speed_Ramp->Preramp)
            {
                Speed_Ref_Output = Speed_Ramp->Preramp;//Final value processing
            }
            else;
         }
         else if(Speed_Ref_Output > Speed_Ramp->Preramp)//Acceleration processing
         {
            Speed_Ref_Output -= Speed_Ramp->AccStep;
            if(Speed_Ref_Output < Speed_Ramp->Preramp)//Final value processing
            {
                Speed_Ref_Output = Speed_Ramp->Preramp;
            }
            else;
         }
         else;
     }

    //New speed >= 0, current speed < 0
    else if((Speed_Ref_Output < _IQ24(0.0)) && (Speed_Ramp->Preramp >= _IQ24(0.0)))
    {
        Speed_Ref_Output += Speed_Ramp->DecStep;
    }

    else;
    Speed_Ramp->Aftramp = Speed_Ref_Output;
}
/******************************************************************************
* Function Name  : Power_Ramp_Process
* Description    : Power ramp processing, executed in the loop, the acceleration/deceleration calculation frequency must be the same as the interrupt frequency;
* Input          : Pointer to the ramp structure
* Output         : Power after the ramp processing
* Return         : None
******************************************************************************/
void Power_Ramp_Process(Rampctr_Type *Power_Ramp)
{
    _iq24 Power_Ref_Output;
    Power_Ref_Output = Power_Ramp->Aftramp;
        if(Power_Ref_Output < Power_Ramp->Preramp)//Ramp up processing
        {
            Power_Ref_Output += Power_Ramp->AccStep;
            if(Power_Ref_Output > Power_Ramp->Preramp)//Final value processing
            {
                Power_Ref_Output = Power_Ramp->Preramp;
            }
            else;
        }
        else if(Power_Ref_Output > Power_Ramp->Preramp)//Ramp down processing
        {
            Power_Ref_Output -= Power_Ramp->DecStep;
            if(Power_Ref_Output < Power_Ramp->Preramp)//Final value processing
            {
                Power_Ref_Output = Power_Ramp->Preramp;
            }
            else;
        }
        Power_Ramp->Aftramp = Power_Ref_Output;
}
/******************************************************************************
* Function Name : Poweroperation_Process
* Description   : Power on/off control logic
* Input         : Pointer to the acceleration/deceleration control structure, pointer to the ADC sampling structure, pointer to the power on/off control structure, running status, pointer to the speed PID controller, actual speed
* Output        : None
* Return        : None
******************************************************************************/
void Poweroperation_Process(Rampctr_Type *SpeedRamp,ADCStruc_Type *ADCStruc,Powercontrol_Type *PowerOper,\
                    Runningstatus_Type *Runningstatus,_iq24 Speedact)
{
//Startup finish judgement
    if((_IQabs(SpeedRamp->Aftramp) >= START_FINISH_SPEED)&&(_IQabs(Speedact) >= START_FINISH_SPEED))
        SpeedRamp->Startfinish = TRUE;

//Power-off logic, detected when already powered on
//    if( (PowerOper->DCVoltAver <= PowerOper->Poweroff_Volt)||(PD_DEVICE.ConnectStat==0))//stop immediately to prevent hardware instability at low voltage
    if( (ADC_M.DCVoltAver  <= POWEROFF_VOLT))//stop immediately to prevent hardware instability at low voltage
    {
        switch(*Runningstatus)
        {
            case INIT:
            case DIRCHECK:
            case PRESTART:
            case POSITION:
            case START:
            case RUN:
                if(*Runningstatus != FAULT)
                *Runningstatus = STOP;
                break;
            case IDLE:
            case STOP:
            case BRAKE:
            case WAIT:
            case FAULT: break;
            }

        PowerOper->Poweron = FALSE;
    }


 //Power-on logic, detected when not powered
//    if((PowerOper->Poweron == FALSE)&&(PD_DEVICE.ConnectStat==1))
    if((PowerOper->Poweron == FALSE))
    {
        if( (ADC_M.DCVoltAver > POWERON_VOLT))
        {
            PowerOper->Poweron_Cnt++;
            if(PowerOper->Poweron_Cnt >= PowerOper->Poweron_CntThresh)
            {
                PowerOper->Poweron = TRUE;
                PowerOper->Poweron_Cnt = 0;
            }
        }
    }
}
/******************************************************************************
* Function Name  : BUZZ_TIMER_Init
* Description    : Buzzer control timer initialization
* Input          : None
* Output         : None
* Return         : None
******************************************************************************/
void BUZZ_TIMER_Init(void)
{
    //timer initialization structure used
    TIM_TimeBaseInitTypeDef TimeBaseInitStructure={0};
    TIM_OCInitTypeDef TIM_OCInitStructure={0};
    GPIO_InitTypeDef GPIO_InitStructure={0};

    //clock enable
    RCC_PB1PeriphClockCmd(RCC_PB1Periph_TIM2, ENABLE);
    RCC_PB2PeriphClockCmd(RCC_PB2Periph_GPIOC, ENABLE);

    //buzzer port configuration
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin =GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    //capture timer configuration
    TIM_DeInit(TIM2);
    TIM_TimeBaseStructInit(&TimeBaseInitStructure);
    TimeBaseInitStructure.TIM_Prescaler = BUZZ_TIMER_PRESCALER;
    TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TimeBaseInitStructure.TIM_Period = BUZZ_TIMER_PERIOD;
    TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM2,&TimeBaseInitStructure);

    //channel 2 configuration, used to generate PWM wave
    TIM_OCStructInit(&TIM_OCInitStructure);
    //valid when greater than the compare register value, otherwise invalid
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
    //forward channel compare output enable
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    //capture compare register value is the timer period
    TIM_OCInitStructure.TIM_Pulse = BUZZ_TIMER_PERIOD/2;
    //forward channel compare output active high
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
   //complementary channel idle state is low level
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
    TIM_OC3Init(TIM2, &TIM_OCInitStructure);//capture compare channel 2 initialization
    TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);//enable the preload of capture compare channel 2
    TIM_CCxCmd(TIM2, TIM_Channel_3, TIM_CCx_Disable); //disable the PWM output of channel 2

    //timer interrupt flags
    TIM_ClearFlag(TIM2, TIM_FLAG_Update | TIM_FLAG_CC1 | TIM_FLAG_CC2 | \
                  TIM_FLAG_CC3 | TIM_FLAG_CC4 | TIM_FLAG_Trigger | TIM_FLAG_CC1OF | \
                  TIM_FLAG_CC2OF | TIM_FLAG_CC3OF | TIM_FLAG_CC4OF);

    TIM_ClearFlag(TIM2, TIM_FLAG_Update);      //update event interrupt flag
    TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);//disable capture interrupt
    TIM_SetCounter(TIM2,0);                    //clear the timer counter
}
/******************************************************************************
* Function Name  : Time_Delay_Process
* Description    : Decrement processing of variables based on the interrupt period (0.5ms)
* Input          : None
* Output         : None
* Return         : None
******************************************************************************/
void Time_Delay_Process(void)
{
    if (Timewait != 0)
        Timewait --;

    if (Operation_M.StopwaitCnt != 0)
        Operation_M.StopwaitCnt --;

    if (Operation_M.FaultHoldingCnt != 0)
        Operation_M.FaultHoldingCnt--;

    if ((Phaseloss_M.DetectphaseCnt!=0)&&(RunningStatus_M==RUN))
    {
        Phaseloss_M.DetectphaseCnt--;
        if(Phaseloss_M.DetectphaseCnt==0)
        {
            Phaseloss_M.Detectphase=TRUE;
        }
    }
}

/******************************************************************************
* Function Name  : Time_Wait
* Description    : Wait until the delay expires.
* Input          : None
* Output         : None
* Return         : None
******************************************************************************/
void Time_Wait(uint16_t time)
{
    Timewait = time;
    while (Timewait != 0)
    {;}
}

/******************************************************************************
* Function Name  : Time_Delay_Set
* Description    : delay time setting function
* Input          : delay time, pointer to the delay counter variable
* Output         : None
* Return         : None
******************************************************************************/
void Time_Delay_Set(uint16_t Delayset, uint16_t *Delaycounter)
{
    *Delaycounter = Delayset;
}

/******************************************************************************
* Function Name  : Time_Delay_Elapse
* Description    : delay timeout judgement
* Input          : delay counter variable
* Output         : judgement result, time is up: TRUE, time is not up: FALSE
* Return         : judgement result
******************************************************************************/
Truth_Verify_Type Time_Delay_Elapse(uint16_t Delaycounter)
{
     if (Delaycounter == 0)
       return (TRUE);
     else
       return (FALSE);
}
