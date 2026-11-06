/* Includes -----------------------------------------------------------------*/
#include "M0_Control_Library.h"

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private functions --------------------------------------------------------*/
void ADC1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
extern void FOC_Cal(Motor_Sructure_Type *MStruc,SVPWM_2R_Type *SVPWM_Cal,Motor_PID_Type *Mpid,\
             ADCStruc_Type *ADCStruc,Motor_SMO_Type *SMOM,int8_t Direction,uint8_t ObsLPF_Enable);
/* Private variables --------------------------------------------------------*/
/* Variables ----------------------------------------------------------------*/
/******************************************************************************
* Function Name  : ADC_IRQHandler
* Description    : ADC1 interrupt handler, reads current conversion result
* Input          : None
* Output         : None
* Return         : None
******************************************************************************/
__attribute__((section(".highcode")))
void ADC1_IRQHandler(void)
{
        int16_t ADsample_int1=0;//AD sampling result integer value after limiting
        int16_t ADsample_int2=0;//AD sampling result integer value after limiting

        ADsample_int1 = (int32_t)(ADC_M.OffsetValue1) - (int32_t)((ADC1->IDATAR1)<<1);
        ADsample_int2 = (int32_t)(ADC_M.OffsetValue2) - (int32_t)((ADC1->IDATAR2)<<1);

        //limit
        ADsample_int1=( (ADsample_int1) < (S16_MIN) ? (S16_MIN) : \
                                      ( (ADsample_int1) > (S16_MAX) ? (S16_MAX) : (ADsample_int1)));

        ADsample_int2=( (ADsample_int2) < (S16_MIN) ? (S16_MIN) : \
                                      ( (ADsample_int2) > (S16_MAX) ? (S16_MAX) : (ADsample_int2)));

#if defined (M007E8R6_FAN_1_1)
        MStruc_M.C3S.PhaseB  = (_iq24)((_IQ16MPY_MACRO(_IQ16(ADsample_int1),\
                                        ADC_M.RatedCurr_ConvInv))<<1);//the 128x coefficient magnification must be restored
        MStruc_M.C3S.PhaseC = (_iq24)((_IQ16MPY_MACRO(_IQ16(ADsample_int2),\
                                        ADC_M.RatedCurr_ConvInv))<<1);//the 128x coefficient magnification must be restored
#elif defined (M007E8U6_FAN_1_1)
        MStruc_M.C3S.PhaseC  = (_iq24)((_IQ16MPY_MACRO(_IQ16(ADsample_int1),\
                                        ADC_M.RatedCurr_ConvInv))<<1);//the 128x coefficient magnification must be restored
        MStruc_M.C3S.PhaseB = (_iq24)((_IQ16MPY_MACRO(_IQ16(ADsample_int2),\
                                        ADC_M.RatedCurr_ConvInv))<<1);//the 128x coefficient magnification must be restored
#endif

        //IA calculation IA= -IB-IC
        MStruc_M.C3S.PhaseA= -(MStruc_M.C3S.PhaseC + MStruc_M.C3S.PhaseB);

        switch(RunningStatus_M)
        {
            case DIRCHECK:
   
                //windmill 43us 
                if(Flystart_M.Process_mark==FALSE)
                {
                    Flystart_Process(&Flystart_M,Ramp_M.Direction,&MStruc_M,&SVPWM_Cal_M,&SMO_M,&ADC_M,&Mpid_M);
                }

                if((Flystart_M.Process_mark==TRUE)&&(Flystart_M.Switch_mark==FALSE))
                 {
                     Flystart_Switch(&Flystart_M,&Ramp_M);
                 }
            break;
#ifdef  POSITION_EN
            case POSITION:
                //pulse injection positioning
                HFVQI_PosiSearch_Process(&HFVQI_StartStruc_M,&SVPWM_Cal_M,ADC_M.DCVoltAver,Ramp_M.Direction);
                //injection positioning startup
                if(HFVQI_StartStruc_M.Finish == TRUE)
                {
                    SMO_M.Cal_Struc.PosiElecPU=HFVQI_StartStruc_M.Position;
                    TrigonoMetric_Function(&(SMO_M.Cal_Struc.Triangle), SMO_M.Cal_Struc.PosiElecPU);
                    RunningStatus_M = RUN;

                    MStruc_M.CRef2R.cd=DAXIS_CURR_REF_INIT1_M;
                    MStruc_M.CRef2R.cq=QAXIS_CURR_REF_INIT1_M*Ramp_M.Direction;
                    SMO_M.Cal_Struc.Inimark=TRUE;
                    DCurrProcess_M.Stage = 1;
                }
                break;
#endif
            case RUN:                            
  
                FOC_Cal(&MStruc_M,&SVPWM_Cal_M,&Mpid_M,&ADC_M,&SMO_M,Ramp_M.Direction,OBS_LPF_EN);//FOC calculation
                 
                break;
            default:
              break;
        }

    //PWM compare update first
    TIM1->CH1CVR = SVPWM_Cal_M.cc1Load;
    TIM1->CH2CVR = SVPWM_Cal_M.cc2Load;
    TIM1->CH3CVR = SVPWM_Cal_M.cc3Load;

    /*phase-loss fault judgement*/
    if ( (((RunningStatus_M == RUN)&&(Ramp_M.Decemark==FALSE)) )\
            &&(Phaseloss_M.Detectphase==TRUE) )
    {
        if((Phaseloss_Verify2R3R(&Phaseloss_M,&MStruc_M)))
            Protection_SetFault(PHASELOSS_FALUT);
    }

    /*overcurrent fault judgement*/
    if ((RunningStatus_M == DIRCHECK)||(RunningStatus_M == RUN))
    {
        if(Phase_Ins_OverCurrent_Verify(&(MStruc_M.C3S),&InstPhaseOC_M) == TRUE)
            Protection_SetFault(MOTOR_OVER_CURR_SOFT_M);
    }

    //clear ADC interrupt flag
    ADC1->STATR = ~(uint32_t)ADC_FLAG_JEOC;
            
}

