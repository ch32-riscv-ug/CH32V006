/* Includes -----------------------------------------------------------------*/
#include "M0_Control_Library.h"

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private functions --------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Variables ----------------------------------------------------------------*/
Motor_PID_Type  Mpid_M;                 //motor PID regulator structure
DCurrProcess_Type   DCurrProcess_M;     //motor D-axis current processing structure
Motor_Power_Type Power_Cal;

//limit table definition
#ifdef MAG100_TABLE_USED
const _iq24 Magnitude_table[256] = MAG100_LIMIT_TABLE;
#elif  defined MAG098_TABLE_USED
const _iq24 Magnitude_table[256] = MAG098_LIMIT_TABLE;
#elif  defined MAG096_TABLE_USED
const _iq24 Magnitude_table[256] = MAG096_LIMIT_TABLE;
#elif  defined MAG094_TABLE_USED
const _iq24 Magnitude_table[256] = MAG094_LIMIT_TABLE;
#elif  defined MAG092_TABLE_USED
const _iq24 Magnitude_table[256] = MAG092_LIMIT_TABLE;
#elif  defined MAG090_TABLE_USED
const _iq24 Magnitude_table[256] = MAG090_LIMIT_TABLE;
#elif  defined MAG088_TABLE_USED
const _iq24 Magnitude_table[256] = MAG088_LIMIT_TABLE;
#elif  defined MAG086_TABLE_USED
const _iq24 Magnitude_table[256] = MAG086_LIMIT_TABLE;
#endif

/******************************************************************************
* Function Name  : PID_Prameter_Define
* Description    : Speed regulator and current regulator parameter definition
* Input          : pointer to motor PID regulator structure
* Output         : None
* Return         : None
******************************************************************************/
void PID_Prameter_Define (Motor_PID_Type * Mpid)
{
    //speed regulator parameter initialization
    Mpid->Speed.CalFreq = SPEED_LOOP_CAL_FREQ;
    Mpid->Speed.CalT = SPEED_REGULATOR_PERIOD_M;
    Mpid->Speed.Kpreal= SPEED_REGULATOR_KP_M;
    Mpid->Speed.Kireal= SPEED_REGULATOR_KI_M;
    Mpid->Speed.Kdreal= SPEED_REGULATOR_KD_M;
    Mpid->Speed.Kpadj = SPEED_REGULATOR_KPADJ_M;
    Mpid->Speed.Kiadj = SPEED_REGULATOR_KIADJ_M;
    Mpid->Speed.Kdadj = SPEED_REGULATOR_KDADJ_M;

    if(STARTUP_DIRECTION ==1)
    {
        Mpid->Speed.Upper_Lim = SPEED_REGULATOR_ULIM_M;
        Mpid->Speed.Lower_Lim = SPEED_REGULATOR_LLIM_M;
    }
    else
    {
        Mpid->Speed.Upper_Lim = -SPEED_REGULATOR_LLIM_M;
        Mpid->Speed.Lower_Lim = -SPEED_REGULATOR_ULIM_M;
    }

    Mpid->Speed.Deltalim = SPEED_REGULATOR_DLIM_M;

    //d-axis current regulator parameter initialization
    Mpid->dCurr.CalFreq= CURR_REGULATOR_FREQ_M;
    Mpid->dCurr.CalT= _IQ(1.0)/CURR_REGULATOR_FREQ_M;
    Mpid->dCurr.Bw= CURR_REGULATOR_BW_M;
    Mpid->dCurr.Kdreal= DAXIS_CURR_REGULATOR_KD_M;
    Mpid->dCurr.Kpadj = DAXIS_CURR_REGULATOR_KPADJ_M;
    Mpid->dCurr.Kiadj = DAXIS_CURR_REGULATOR_KIADJ_M;
    Mpid->dCurr.Kdadj = DAXIS_CURR_REGULATOR_KDADJ_M;
    Mpid->dCurr.Upper_Lim = DAXIS_CURR_REGULATOR_ULIM_M;
    Mpid->dCurr.Lower_Lim = DAXIS_CURR_REGULATOR_LLIM_M;
    Mpid->dCurr.Deltalim = DAXIS_CURR_REGULATOR_DLIM_M;

    //q-axis current regulator parameter initialization
    Mpid->qCurr.CalFreq= CURR_REGULATOR_FREQ_M;
    Mpid->qCurr.CalT= _IQ(1.0)/CURR_REGULATOR_FREQ_M;
    Mpid->qCurr.Bw= CURR_REGULATOR_BW_M;
    Mpid->qCurr.Kdreal= QAXIS_CURR_REGULATOR_KD_M;
    Mpid->qCurr.Kpadj = QAXIS_CURR_REGULATOR_KPADJ_M;
    Mpid->qCurr.Kiadj = QAXIS_CURR_REGULATOR_KIADJ_M;
    Mpid->qCurr.Kdadj = QAXIS_CURR_REGULATOR_KDADJ_M;
    Mpid->qCurr.Upper_Lim = QAXIS_CURR_REGULATOR_ULIM_M;
    Mpid->qCurr.Lower_Lim = QAXIS_CURR_REGULATOR_LLIM_M;
    Mpid->qCurr.Deltalim = QAXIS_CURR_REGULATOR_DLIM_M;
}
/******************************************************************************
* Function Name  : PID_ParaBase_Cal_M
* Description    : Speed regulator and current regulator parameter calculation
* Input          : pointer to motor base value parameter structure, pointer to motor PID regulator structure
* Output         : None
* Return         : None
******************************************************************************/
void PID_ParaBase_Cal_M(Motor_Base_Type *Basestruc,Motor_PID_Type * Mpid)
{
    //speed regulator proportional coefficient per-unit value
    Mpid->Speed.Kp = _IQ16toIQ(_IQ16div(_IQtoIQ16(Mpid->Speed.Kpreal * Basestruc->Wmbase),Basestruc->Tbase));
    Mpid->Speed.Kp = _IQmpy(Mpid->Speed.Kp,Mpid->Speed.Kpadj);

    //speed regulator integral coefficient per-unit value
    Mpid->Speed.Ki = _IQmpy(_IQdiv(Mpid->Speed.Kireal,Basestruc->Tbase<<8) * Basestruc->Wmbase,Mpid->Speed.CalT);
    Mpid->Speed.Ki = _IQmpy(Mpid->Speed.Ki,Mpid->Speed.Kiadj);

    //speed regulator derivative coefficient per-unit value
    Mpid->Speed.Kd = _IQ16toIQ(_IQ16div(_IQtoIQ16((Mpid->Speed.Kdreal*Mpid->Speed.CalFreq) * Basestruc->Wmbase),Basestruc->Tbase));
    Mpid->Speed.Kd = _IQmpy(Mpid->Speed.Kd,Mpid->Speed.Kdadj);

    //D-axis current regulator proportional coefficient per-unit value
    Mpid->dCurr.Kp = _IQmpy(Basestruc->Ld * Mpid->dCurr.Bw,_IQ16toIQ(_IQ16div(Basestruc->Ibase,Basestruc->Vbase)));
    Mpid->dCurr.Kp = _IQMPY_MACRO(Mpid->dCurr.Kp,Mpid->dCurr.Kpadj);

    //D-axis current regulator integral coefficient per-unit value
    Mpid->dCurr.Ki = _IQmpy((_IQmpy(Basestruc->Rstotal,Mpid->dCurr.CalT*Mpid->dCurr.Bw)),_IQ16toIQ(_IQ16div(Basestruc->Ibase,Basestruc->Vbase)));
    Mpid->dCurr.Ki = _IQmpy(Mpid->dCurr.Ki,Mpid->dCurr.Kiadj);

    //D-axis current regulator derivative coefficient per-unit value
    Mpid->dCurr.Kd = _IQmpy((Mpid->dCurr.Kdreal * Mpid->dCurr.CalFreq),_IQ16toIQ(_IQ16div(Basestruc->Ibase,Basestruc->Vbase)));
    Mpid->dCurr.Kd = _IQmpy(Mpid->dCurr.Kd,Mpid->dCurr.Kdadj);

    //Q-axis current regulator proportional coefficient per-unit value
    Mpid->qCurr.Kp = _IQmpy((Basestruc->Lq*Mpid->qCurr.Bw),_IQ16toIQ(_IQ16div(Basestruc->Ibase,Basestruc->Vbase)));
    Mpid->qCurr.Kp = _IQmpy(Mpid->qCurr.Kp,Mpid->qCurr.Kpadj);

    //Q-axis current regulator integral coefficient per-unit value
    Mpid->qCurr.Ki = _IQmpy((_IQmpy(Basestruc->Rstotal,(Mpid->qCurr.CalT*Mpid->qCurr.Bw))),_IQ16toIQ(_IQ16div(Basestruc->Ibase,Basestruc->Vbase)));
    Mpid->qCurr.Ki = _IQmpy(Mpid->qCurr.Ki,Mpid->qCurr.Kiadj);

    //Q-axis current regulator derivative coefficient per-unit value
    Mpid->qCurr.Kd = _IQmpy((Mpid->qCurr.Kdreal * Mpid->qCurr.CalFreq),_IQ16toIQ(_IQ16div(Basestruc->Ibase,Basestruc->Vbase)));
    Mpid->qCurr.Kd = _IQmpy(Mpid->qCurr.Kd,Mpid->qCurr.Kdadj);
}

/******************************************************************************
* Function Name  : PID_Output_Init
* Description    : Speed regulator and current regulator output state initialization.
* Input          : pointer to motor PID regulator structure
* Output         : None
* Return         : None
******************************************************************************/
void PID_Output_Init(Motor_PID_Type * Mpid, int8_t Direction)
{
    //speed regulator output state initialization
    Mpid->Speed.Err_New= 0;
    Mpid->Speed.Err_Prev1= 0;
    Mpid->Speed.Err_Prev2= 0;
    Mpid->Speed.Output_New = 0;
    Mpid->Speed.Output_Prev = 0;

    Mpid->Speed.Deltaoutput = 0;
    Mpid->Speed.Differential = 0;

    //d-axis current regulator output state initialization
    Mpid->dCurr.Err_New= 0;
    Mpid->dCurr.Err_Prev1= 0;
    Mpid->dCurr.Err_Prev2= 0;
    Mpid->dCurr.Output_New= 0;
    Mpid->dCurr.Output_Prev= 0;
    Mpid->dCurr.Deltaoutput =0;

    //q-axis current regulator output state initialization
    Mpid->qCurr.Err_New= 0;
    Mpid->qCurr.Err_Prev1= 0;
    Mpid->qCurr.Err_Prev2= 0;
    Mpid->qCurr.Output_New = 0;
    Mpid->qCurr.Output_Prev = 0;
    Mpid->qCurr.Deltaoutput = 0;

    //for forward/reverse rotation switching
    if(Direction ==1)
    {
        Mpid->Speed.Upper_Lim = SPEED_REGULATOR_ULIM_M;
        Mpid->Speed.Lower_Lim = SPEED_REGULATOR_LLIM_M;
    }
    else
    {
        Mpid->Speed.Upper_Lim = -SPEED_REGULATOR_LLIM_M;
        Mpid->Speed.Lower_Lim = -SPEED_REGULATOR_ULIM_M;
    }
}
/******************************************************************************
* Function Name  : DCurrProcess_Parameter_Define
* Description    : d-axis current processing related parameter definition
* Input          : d-axis current processing structure
* Output         : None
* Return         : None
******************************************************************************/
void DCurrProcess_Parameter_Define(DCurrProcess_Type * DCurrProcess)
{
    DCurrProcess->Target1 = DAXIS_CURR_REF_INIT1_M;
    DCurrProcess->Target2 = DAXIS_CURR_REF_INIT2_M;
    DCurrProcess->AdjustStep1 = DAXIS_CURR_REF_STEP1_M;
    DCurrProcess->AdjustStep2 = DAXIS_CURR_REF_STEP2_M;
    DCurrProcess->Weakpoint = DAXIS_CURR_WEAK_SPEED_M;
}
/******************************************************************************
* Function Name  : PowerCal_Prameter_Define
* Description    : Power loop parameter configuration.
* Input          : None
* Output         : None
* Return         : None
******************************************************************************/
void PowerCal_Prameter_Define(Motor_Power_Type *MPower )
{
    MPower->Power.Kp=POWER_REGULATOR_KP_M;
    MPower->Power.Ki = _IQmpy(POWER_REGULATOR_KI_M,POWER_REGULATOR_PERIOD_M);
    MPower->Power.Upper_Lim = POWER_REGULATOR_ULIM_M;
    MPower->Power.Lower_Lim = POWER_REGULATOR_LLIM_M;
    MPower->Power.Deltalim  = POWER_REGULATOR_DLIM_M;

    //speed filter parameter calculation used by observer
    MPower->PowerLPF1st.Input_Coef = _IQdiv(POWERLPF_WcT_M,(POWERLPF_WcT_M + UNIT_Q24));
    MPower->PowerLPF1st.Output_Coef = _IQdiv(UNIT_Q24,(POWERLPF_WcT_M + UNIT_Q24));

}
/******************************************************************************
* Function Name  : PowerCal_Init
* Description    : Power loop initialization.
* Input          : None
* Output         : None
* Return         : None
******************************************************************************/
void PowerCal_Init(Motor_Power_Type *MPower)
{
    MPower->Power.Err_New= 0;
    MPower->Power.Err_Prev1= 0;
    MPower->Power.Err_Prev2= 0;
    MPower->Power.Output_New = 0;
    MPower->Power.Output_Prev = 0;

    MPower->Power.Deltaoutput = 0;
    MPower->Power.Differential = 0;

    MPower->PowerLPF1st.Out_New = _IQ(0.0);
    MPower->PowerLPF1st.Out_Pre = _IQ(0.0);

    MPower->AngPower_Aver1 = _IQ(0.0);
    MPower->AngPower_Inst  = _IQ(0.0);
}
