/* Includes ------------------------------------------------------------------*/
#include "M0_Control_Library.h"

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/

Motor_SMO_Type SMO_M;                          //SMO observer structure
Observer_Status_Type    SMO_Status_M;           //SMO observer status structure
_iq24 SMO_SpeedBuffer_M[SMO_SPEED_BUFFERSIZE_M];//SMO observer observed speed average calculation array
/******************************************************************************
* Function Name : SMO_Parameter_Define
* Description   : MSMO observer parameter definition
* Input         : pointer to MSMO observer structure, pointer to MSMO observer status structure
* Output        : None
* Return        : None
******************************************************************************/
void SMO_Parameter_Define (Motor_SMO_Type *MSMO,Observer_Status_Type *SMOStatus)
{
    //MSMO observer calculation parameters
    MSMO->Para.CalFreq = SMO_CAL_FREQ_M;
    MSMO->Para.CalT = SMO_CAL_PERIOD_M;

    MSMO->Cal_Struc.Settling_Set=SMO_CAL_FREQ_M * SMO_SETTING_TIME_M;

    MSMO->Para.Kslide1=SMO_CERROR1_EMF_M;
    MSMO->Para.Kslide2=SMO_CERROR2_EMF_M;
    MSMO->Para.MaxSMCError=SMO_MAXSMCERROR;

    //MSMO observer parameters
    MSMO->Cal_Struc.EUlim = SMO_MAX_EMF_M;
    MSMO->Cal_Struc.ELlim = SMO_MIN_EMF_M;
    MSMO->Cal_Struc.CUlim = SMO_MAX_CURR_M;
    MSMO->Cal_Struc.CLlim = SMO_MIN_CURR_M;

    //speed filter parameter calculation used by observer
    MSMO->Cal_Struc.SpeedLPF1st_M.Input_Coef = _IQdiv(SMO_SPEEDLPF_WcT_M,(SMO_SPEEDLPF_WcT_M + UNIT_Q24));
    MSMO->Cal_Struc.SpeedLPF1st_M.Output_Coef = _IQdiv(UNIT_Q24,(SMO_SPEEDLPF_WcT_M + UNIT_Q24));

    //MSMO observer PLL parameters
    MSMO->Speed_PLL.Zeta=PLL_ZETA;
    MSMO->Speed_PLL.Bandwidth=_IQ16((float)(PLL_BAND)/RATED_FREQ_M)<<8;
    MSMO->Speed_PLL.SpeedLim =PLL_SPEED_DLIM;
    MSMO->Speed_PLL.SpeedLim1 =PLL_SPEED_DLIM1;
    MSMO->Speed_PLL.Upper_Lim = SMO_SPEED_MAX_M * STARTUP_DIRECTION;
    MSMO->Speed_PLL.Lower_Lim = SMO_SPEED_MIN_M * STARTUP_DIRECTION;

    //MSMO observer speed average calculation parameters
    MSMO->SpeedBuffPara.Length = SMO_SPEED_BUFFERSIZE_M;

    //MSMO observer status judgement parameters
    SMOStatus->OverspeedThresh = SMO_OVERSPEED_THRESH_M;
    SMOStatus->OverspeedcntThresh = SMO_OVERSPEED_CNT_THRESH_M;
    SMOStatus->UnderspeedThresh = SMO_UNDERSPEED_THRESH_M;
    SMOStatus->UnderspeedcntThresh = SMO_UNDERSPEED_CNT_THRESH_M;
    SMOStatus->VarianceThresh = SMO_SPEED_VARIANCE_THRESH_M;
    SMOStatus->UnstableCntThresh = SMO_UNSTABLE_CNT_THRESH_M;
    SMOStatus->UnstableSpeedThresh = SMO_UNSTABLE_SPEED_THRESH_M;
    SMOStatus->StallSpeedBand = SMO_STALLBAND_M;
    SMOStatus->StallSpeed = SMO_STALLSPEED_M;
    SMOStatus->StallCurr =  SMO_STALLCURR_M;
    SMOStatus->StallCntThresh = SMO_STALL_TIME_THRESH_M*SPEED_LOOP_CAL_FREQ;
}

/******************************************************************************
* Function Name : SMO_Status_Init
* Description   : SMO observer status initialization
* Input         : pointer to SMO observer structure, pointer to SMO observer status structure
* Output        : None
* Return        : None
******************************************************************************/
void SMO_Status_Init (Motor_SMO_Type *MSMO,Observer_Status_Type *SMOStatus,int8_t Direction)
{
    //MSMO observer variable initialization
    MSMO->Cal_Struc.Inimark = FALSE;

    MSMO->Cal_Struc.EalphaNew = _IQ(0.0);
    MSMO->Cal_Struc.EalphaPrev = _IQ(0.0);

    MSMO->Cal_Struc.EbetaNew = _IQ(0.0);
    MSMO->Cal_Struc.EbetaPrev  = _IQ(0.0);

    MSMO->Cal_Struc.CalphaNew = _IQ(0.0);
    MSMO->Cal_Struc.CalphaPrev = _IQ(0.0);
    MSMO->Cal_Struc.CbetaNew = _IQ(0.0);
    MSMO->Cal_Struc.CbetaPrev = _IQ(0.0);

    MSMO->Cal_Struc.ThetaErr = _IQ(0.0);
    MSMO->Cal_Struc.PosiElecPU = _IQ(0.0);
    TrigonoMetric_Function(&(MSMO->Cal_Struc.Triangle),MSMO->Cal_Struc.PosiElecPU);
    MSMO->Cal_Struc.PosiElecPU_Cali = _IQ(0.0);
    MSMO->Cal_Struc.PosiElecPU_Com = _IQ(0.0);
    MSMO->Cal_Struc.Angspeed_Inst = _IQ(0.0);
    MSMO->Cal_Struc.Angspeed_Aver1 = _IQ(0.0);
    MSMO->Cal_Struc.Angspeed_Aver2 = _IQ(0.0);
    MSMO->Cal_Struc.Triangle.Cos = _IQ(0.0);
    MSMO->Cal_Struc.Triangle.Sin = _IQ(0.0);

    // MSMO->Cal_Struc.EmfErr = 0;

    //MSMO observer status initialization
    SMOStatus->Overspeed = FALSE;
    SMOStatus->Underspeed = FALSE;
    SMOStatus->Reliable = FALSE;
    SMOStatus->Stable = TRUE;
    SMOStatus->Stall = FALSE;

    SMOStatus->Overspeed_CNT =0;
    SMOStatus->Underspeed_CNT = 0;
    SMOStatus->Unstable_CNT = 0;
    SMOStatus->Stall_CNT = 0;

    //phase-locked loop initialization
    MSMO->Speed_PLL.Error_New = _IQ(0.0);
    MSMO->Speed_PLL.Error_Prev = _IQ(0.0);
    MSMO->Speed_PLL.Output_New = SMO_SPEED_MIN_M * Direction;
    MSMO->Speed_PLL.Output_Prev = SMO_SPEED_MIN_M * Direction;
    MSMO->Cal_Struc.Angspeed_Inst = SMO_SPEED_MIN_M * Direction;


    MSMO->Cal_Struc.SpeedLPF1st_M.Out_New=_IQ(0.0);
    MSMO->Cal_Struc.SpeedLPF1st_M.Out_Pre=_IQ(0.0);

    MSMO->Speed_PLL.Upper_Lim = SMO_SPEED_MAX_M * Direction;
    MSMO->Speed_PLL.Lower_Lim = SMO_SPEED_MIN_M * Direction;
    MSMO->Speed_PLL.Vslim = 0;
    MSMO->Cal_Struc.Settling_Cnt=0;

}

