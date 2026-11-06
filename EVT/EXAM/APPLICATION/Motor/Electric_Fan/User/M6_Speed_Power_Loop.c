/* Includes -----------------------------------------------------------------*/
#include "M0_Control_Library.h"

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private functions --------------------------------------------------------*/
void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

/* Private variables --------------------------------------------------------*/
/* Variables ----------------------------------------------------------------*/
/******************************************************************************
* Function Name  : SysTick_Handler
* Description    : Speed loop processing interrupt.
* Input          : None
* Output         : None
* Return         : None
******************************************************************************/
void SysTick_Handler(void)
{
    _iq CRef2R_cq=0;
    //Variable decrement processing based on the interrupt period (0.5ms)
    Time_Delay_Process();

    //Power on/off control processing logic
    Poweroperation_Process(&Ramp_M,&ADC_M,&Powerctr_M,&RunningStatus_M,SMO_M.Cal_Struc.Angspeed_Aver1);

    //Keys are only available after the power on is effective
   if((Powerctr_M.Poweron==TRUE))
    {
        //Key detection
        KeyCheck_Process(&Key_M,&Ramp_M);
    }

    switch(RunningStatus_M)
    {
        case RUN:
        
            //Update the SMO observed speed buffer
            Buffer_Update(&(SMO_SpeedBuffer_M[0]), &(SMO_M.SpeedBuffPara), SMO_M.Cal_Struc.Angspeed_Inst);

            //Calculate the SMO observed average speed
            SMO_M.Cal_Struc.Angspeed_Aver1 = LPF_1st_Process(&(SMO_M.Cal_Struc.SpeedLPF1st_M),\
                    SMO_M.Cal_Struc.Angspeed_Inst);//Low-pass filter of the observed speed

//             //Stall judgement
            if((Ramp_M.Decemark==FALSE))
            {
                Motor_Stall_Verify(Ramp_M.Aftramp, _IQMPY_MACRO(SMO_M.Cal_Struc.EalphaNew,SMO_M.Para.kEV),_IQMPY_MACRO(SMO_M.Cal_Struc.EbetaNew,SMO_M.Para.kEV), MStruc_M.CRef2R.cq,&SMO_Status_M,SMO_M.Cal_Struc.Angspeed_Aver2);
                if(SMO_Status_M.Stall==TRUE)
                 Protection_SetFault(MOTOR_STALL_M);

                Speed_Outrange_Verify(_IQabs(SMO_M.Cal_Struc.Angspeed_Aver1),&SMO_Status_M);
                //Underspeed protection triggers the stall protection
                    if(SMO_Status_M.Underspeed==TRUE)
                        Protection_SetFault(MOTOR_STALL_M);
             }

            //Speed loop regulation
#if     (CLOSE_LOOP==SPEED_LOOP)
                Speed_Ramp_Process(&Ramp_M);
                CRef2R_cq = PI_Calculaion_Delta (Ramp_M.Aftramp, SMO_M.Cal_Struc.Angspeed_Aver1, &(Mpid_M.Speed));
#elif   (CLOSE_LOOP==POWER_LOOP)
                Power_Cal.AngPower_Inst= _IQabs(_IQMPY_MACRO(MStruc_M.VRef2RC.cd,MStruc_M.C2R.cd))+_IQabs(_IQMPY_MACRO(MStruc_M.VRef2RC.cq,MStruc_M.C2R.cq));
                Power_Cal.AngPower_Aver1 = LPF_1st_Process(&(Power_Cal.PowerLPF1st),Power_Cal.AngPower_Inst);//Low-pass filter of the power
                Power_Ramp_Process(&Ramp_M);
                CRef2R_cq = PI_Calculaion_Delta (Ramp_M.Aftramp, Power_Cal.AngPower_Aver1, &(Power_Cal.Power));
                CRef2R_cq=CRef2R_cq*Ramp_M.Direction;
#endif

            //The set speed is consistent with the underspeed one; staying too long triggers the underspeed protection
            if((_IQabs(SMO_M.Cal_Struc.Angspeed_Aver2)<SMO_UNDERSPEED_THRESH_M)&&(Ramp_M.Decemark==FALSE))
            {
                MStruc_M.CRef2R.cq=_IQ(0.35)*Ramp_M.Direction;

#if     (CLOSE_LOOP==SPEED_LOOP)
                Mpid_M.Speed.Output_New=_IQ(0.35)*Ramp_M.Direction;
                Mpid_M.Speed.Output_Prev=_IQ(0.35)*Ramp_M.Direction;
#elif   (CLOSE_LOOP==POWER_LOOP)
                Power_Cal.Power.Output_New=_IQ(0.35);
                Power_Cal.Power.Output_Prev=_IQ(0.35);
#endif
            }
            else
                MStruc_M.CRef2R.cq=CRef2R_cq;

            //d axis current switching processing
            if((DCurrProcess_M.Stage == 1)&&(_IQabs(MStruc_M.CRef2R.cd) <= DCurrProcess_M.Target1)&&(_IQabs(SMO_M.Cal_Struc.Angspeed_Aver1)>=(_IQ(0.1))))
                Dcurr_Process_Init(&DCurrProcess_M,2);

            MStruc_M.CRef2R.cd = Dcurr_Process(&DCurrProcess_M, SMO_M.Cal_Struc.Angspeed_Aver1, MStruc_M.CRef2R.cd);

            break;
        default:
                break;
    }


    //Bus voltage overvoltage
    DCOvervolt_Protection(ADC_M.DCVoltAver, &DCBusProtecion_M);

    //Bus voltage undervoltage
    DCUndervolt_Protection(ADC_M.DCVoltAver, &DCBusProtecion_M);

    SysTick->SR &= ~(1 << 0);
}

/************************************************************************************************************
* Function Name  : Systick_Init
* Description    : SYS timer initialization
* Input          : NewState - ENABLE or DISABLE.
* Output         : None
* Return         : None
*************************************************************************************************************/
void Systick_Init(void)
{
    SysTick->CTLR  =0;
    SysTick->CMP   =0;
    SysTick->SR   &= ~(1 << 0);
    SysTick->CNT   = 0;
    SysTick->CMP   = TIMER_PERIDO_SYSTICK;  //Compare value
    SysTick->CTLR |=(1 << 1)|(1 << 3)|(1 << 5);//Enable interrupt, auto reload enable, count up, initial value update
}
/************************************************************************************************************
* Function Name  : SYS_TIM_Cmd
* Description    : SYS timer enable
* Input          : NewState - ENABLE or DISABLE.
* Output         : None
* Return         : None
*************************************************************************************************************/
void SYS_TIM_Cmd(FunctionalState NewState)
{
    if(NewState != DISABLE)
    {
        SysTick->CTLR |= TIM_CEN;
    }
    else
    {
        SysTick->CTLR &= (uint16_t)(~((uint16_t)TIM_CEN));
    }
}


