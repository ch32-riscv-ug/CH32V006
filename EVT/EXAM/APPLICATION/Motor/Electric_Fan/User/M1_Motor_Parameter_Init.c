/* Includes -----------------------------------------------------------------*/
#include "M0_Control_Library.h"

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private functions --------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Variables ----------------------------------------------------------------*/
Motor_Sructure_Type MStruc_M;           //motor overall state structure
Motor_Base_Type  MBase_M;               //motor base value parameter structure
Runningstatus_Type  RunningStatus_M;    //system running state
uint16_t volatile System_Status_Global; //system status variable
/******************************************************************************
* Function Name  : Motor_Prameter_Define
* Description    : Motor parameter definition
* Input          : pointer to motor base value parameter structure
* Output         : None
* Return         : None
******************************************************************************/
void Motor_Prameter_Define(Motor_Base_Type *Basestruc)
{
    //motor parameter assignment
    Basestruc->Fn = RATED_FREQ_M;
    Basestruc->Vn = RATED_VOLT_M;
    Basestruc->In = RATED_CURR_M;
    Basestruc->Flux = ROTOR_FLUX_M;
    Basestruc->Pn = POLEPARE_M;
    Basestruc->Rs = STATOR_RES_M;
    Basestruc->Ld = STATOR_LD_M;
    Basestruc->Lq = STATOR_LQ_M;
    Basestruc->Rstotal = STATOR_RES_TOTAL_M;
}

/******************************************************************************
* Function Name  : Motor_Status_Initial_M
* Description    : Motor status initialization
* Input          : pointer to motor overall state structure
* Output         : None
* Return         : None
******************************************************************************/
void Motor_Status_Initial_M(Motor_Sructure_Type *MStruc)
{
    //initialize other motor state variables
    MStruc->VRef2R.cd = _IQ(0.0);
    MStruc->VRef2R.cq = _IQ(0.0);
    MStruc->VRef2RC.cd = _IQ(0.0);
    MStruc->VRef2RC.cq = _IQ(0.0);
    MStruc->VRef2S.Alpha = _IQ(0.0);
    MStruc->VRef2S.Beta = _IQ(0.0);
    MStruc->VRef2SC.Alpha = _IQ(0.0);
    MStruc->VRef2SC.Beta = _IQ(0.0);
    MStruc->CRef2R.cd = _IQ(0.0);
    MStruc->CRef2R.cq = _IQ(0.0);
    MStruc->C3S.PhaseA = _IQ(0.0);
    MStruc->C3S.PhaseB = _IQ(0.0);
    MStruc->C3S.PhaseC = _IQ(0.0);
    MStruc->C2S.Alpha = _IQ(0.0);
    MStruc->C2S.Beta = _IQ(0.0);

    MStruc->C2R_H.cd = _IQ(0.0);
    MStruc->C2R_H.cq = _IQ(0.0);
    MStruc->C2R.cd = _IQ(0.0);
    MStruc->C2R.cq = _IQ(0.0);

    //initialize observed d-axis current low-pass filter
     MStruc->IdLPF1stH.Out_Pre = _IQ(0.0);
     MStruc->IdLPF1stH.Out_New = _IQ(0.0);

    //initialize observed q-axis current low-pass filter
     MStruc->IqLPF1stH.Out_Pre = _IQ(0.0);
     MStruc->IqLPF1stH.Out_New = _IQ(0.0);
     MStruc->C2R_Obs.cd = _IQ(0.0);
     MStruc->C2R_Obs.cq = _IQ(0.0);
     MStruc->C2S_Obs.Alpha = _IQ(0.0);
     MStruc->C2S_Obs.Beta = _IQ(0.0);
}

/******************************************************************************
* Function Name  : Global_Status_Initial
* Description    : System status and motor running state initialization
* Input          : None
* Output         : None
* Return         : None
******************************************************************************/
void Global_Status_Initial(void)
{
    System_Status_Global |= SPEED_CONTROL_M;  //system status initialization speed mode
    RunningStatus_M = IDLE;                   //system status initialization
}

/******************************************************************************
* Function Name  : Motor_FilerPara_Cal_M
* Description    : Motor phase filter parameter initialization
* Input          : pointer to motor overall state structure
* Output         : None
* Return         : None
******************************************************************************/
void Motor_FilerPara_Cal_M(Motor_Sructure_Type *MStruc)
{

  //initialize observed d-axis current low-pass filter parameters, used for feedback (processed in current loop, period not specially handled)
    MStruc->IdLPF1stH.Input_Coef  = IDQLPF_OBS_INPUT_COEF;
    MStruc->IdLPF1stH.Output_Coef = IDQLPF_OBS_OUTPUT_COEF;

   //initialize observed q-axis current low-pass filter parameters, used for feedback (processed in current loop, period not specially handled)
    MStruc->IqLPF1stH.Input_Coef = IDQLPF_OBS_INPUT_COEF;
    MStruc->IqLPF1stH.Output_Coef = IDQLPF_OBS_OUTPUT_COEF;
}

/******************************************************************************
* Function Name  : Motor_BaseCal_M
* Description    : Motor parameter per-unit calculation
* Input          : pointer to motor base value parameter structure
* Output         : None
* Return         : None
******************************************************************************/
void Motor_BaseCal_M(Motor_Base_Type *Basestruc)
{
    //motor electrical angular speed base value (unit-rad/s)
    Basestruc->Webase = (uint16_t)(((_iq16)(Basestruc->Fn)*PI_2_Q16)>>16);

    //motor mechanical angular speed base value (unit-rad/s)
    Basestruc->Wmbase = (uint16_t)(Basestruc->Webase/Basestruc->Pn);

    //motor voltage base value (phase amplitude, unit-V)
    Basestruc->Vbase = _IQ16mpy(Basestruc->Vn,_IQtoIQ16(SQRT_2DIV3_Q24));

    //motor back-EMF base value (phase amplitude, unit-V);
    Basestruc->Ebase = (_IQtoIQ16(Basestruc->Flux)*Basestruc->Webase);

    //motor current base value (phase amplitude, unit-A);
    Basestruc->Ibase = _IQ16mpy(Basestruc->In,_IQtoIQ16(SQRT2_Q24));

    //torque coefficient (unit-N.m/A);
    Basestruc->Tcoef = (_IQmpy(ONE_HALF_Q24,Basestruc->Flux)*Basestruc->Pn);

    //motor torque base value (unit-N.m)
    Basestruc->Tbase = _IQ16mpy(_IQtoIQ16(Basestruc->Tcoef),Basestruc->Ibase);
}
