/* Includes -----------------------------------------------------------------*/
#include "M0_Control_Library.h"

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private functions --------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Variables ----------------------------------------------------------------*/
InstPhaseOC_Type  InstPhaseOC_M;            //AC instantaneous overcurrent software protection
DCbusProtection_Type DCBusProtecion_M;      //Bus voltage protection
Phaseloss_Type Phaseloss_M;                 //Phase loss protection structure

extern _iq24 LPF_1st_Process(LPF_First_Order_Type *LPF_1st, _iq24 input);
extern _iq24 AverValue_Cal(_iq24 Input,_iq24 Output, BuffPara_Type *Buffpara);

/******************************************************************************
* Function Name  : Protection_and_Emergency_Define
* Description    : Fault protection parameter definition
* Input          : None
* Output         : None
* Return         : None
******************************************************************************/
void Protection_and_Emergency_Define (void)
{
    InstPhaseOC_M.Cnt = 0;
    InstPhaseOC_M.CurrThreshold = INST_PHASEOC_THRE_M;
    InstPhaseOC_M.CntThreshold = INST_PHASEOC_CNT_THRE_M;

    DCBusProtecion_M.OverThresh = DCBUS_OVER_VOLT_THRE;
    DCBusProtecion_M.UnderThresh = DCBUS_UNDER_VOLT_THRE;
    DCBusProtecion_M.OverCntThresh = DCBUS_OVER_VOLT_CNT;
    DCBusProtecion_M.UnderCntThresh = DCBUS_UNDER_VOLT_CNT;
    DCBusProtecion_M.OverCnt = 0;
    DCBusProtecion_M.UnderCnt = 0;
    DCBusProtecion_M.Overvolt = FALSE;
    DCBusProtecion_M.Undervolt = FALSE;

    Phaseloss_M.CurrThresh = PHASELOSS_CURR_THRE;
    Phaseloss_M.CntThresh = (uint16_t)(PHASELOSS_TIME_THRE * PHASELOSS_CAL_FREQ);
    Phaseloss_M.MultiphaseCnt = 0;
    Phaseloss_M.SiglephaseCntA = 0;
    Phaseloss_M.SiglephaseCntB = 0;
    Phaseloss_M.SiglephaseCntC = 0;
    Phaseloss_M.CurrBuffPara.Length = PHASELOSS_CAL_BUFFSIZE;
    Phaseloss_M.CurrBuffPara.Inv1 = _IQ(1.0)/PHASELOSS_CAL_BUFFSIZE;
    Phaseloss_M.CurrBuffPara.Inv2 = _IQ(1.0) - Phaseloss_M.CurrBuffPara.Inv1;

    Phaseloss_M.DetectphaseSet = (uint16_t)(SPEED_LOOP_CAL_FREQ*PHASELOSS_DETECT_TIME);
}
/******************************************************************************
* Function Name  : Protection_and_Emergency_Init
* Description    : Fault protection parameter initialization
* Input          : None
* Output         : None
* Return         : None
******************************************************************************/
void Protection_and_Emergency_Init (void)
{
    InstPhaseOC_M.Cnt = 0;
    DCBusProtecion_M.OverCnt = 0;
    DCBusProtecion_M.UnderCnt = 0;
    DCBusProtecion_M.Overvolt = FALSE;
    DCBusProtecion_M.Undervolt = FALSE;

    Phaseloss_M.MultiphaseCnt = 0;
    Phaseloss_M.SiglephaseCntA = 0;
    Phaseloss_M.SiglephaseCntB = 0;
    Phaseloss_M.SiglephaseCntC = 0;
    Phaseloss_M.DetectphaseCnt = Phaseloss_M.DetectphaseSet;
    Phaseloss_M.Detectphase=FALSE;


}
/******************************************************************************
* Function Name  : Protection_SetFault
* Description    : Fault setting
* Input          : Fault type
* Output         : None
* Return         : None
******************************************************************************/
void Protection_SetFault(uint16_t Fault_type)
{
    //Set the fault holding time, which is judged before the fault is cleared (clearing too fast may not be readable)
    Time_Delay_Set(Operation_M.FaultHoldingSet,&(Operation_M.FaultHoldingCnt));

    TIM_CtrlPWMOutputs(TIM1, DISABLE);  //Stop the PWM output

    ADC_Start(DISABLE);//ADC start

    Operation_M.Checkmode =0;

    RunningStatus_M = FAULT;            //Set the motor running state to fault

    System_Status_Global |= Fault_type; //Set the fault bit of the global variable
}

/******************************************************************************
* Function Name  : Protction_Fault_Reset_M
* Description    : Fault reset
* Input          : None
* Output         : None
* Return         : None
******************************************************************************/
void Protction_Fault_Reset_M(void)
{
    if (Time_Delay_Elapse(Operation_M.FaultHoldingCnt))         //Set delay time reached
    {
        System_Status_Global &= ~STARTUP_FAILURE_M;         //Clear the startup failure fault

        System_Status_Global &= ~SPEED_ABNORMAL_M;          //Clear the speed detection abnormal fault
        SMO_Status_M.Unstable_CNT = 0;

        System_Status_Global &= ~MOTOR_OVER_CURR_SOFT_M;    //Clear the software overcurrent fault
        InstPhaseOC_M.Cnt = 0;                              //Clear the software overcurrent count

        if (GPIO_ReadInputDataBit(BRK_GPIO_M, BRK_PIN_M))   //No hardware overcurrent fault
          System_Status_Global &= ~DC_OVER_CURR_HARD_M;     //Clear the fault flag

        System_Status_Global &= ~MOTOR_STALL_M;             //Clear the stall abnormal fault
        SMO_Status_M.Stall_CNT = 0;

        System_Status_Global &= ~PHASELOSS_FALUT;           //Clear the phase loss abnormal fault
        Phaseloss_M.MultiphaseCnt = 0;
        Phaseloss_M.SiglephaseCntA = 0;
        Phaseloss_M.SiglephaseCntB = 0;
        Phaseloss_M.SiglephaseCntC = 0;

        if((System_Status_Global&DC_OVER_VOLT)==DC_OVER_VOLT)       //For the bus overvoltage fault
        {
            if(ADC_M.DCVoltAver < DCBusProtecion_M.OverThresh)   //Check whether it is lower than the bus overvoltage threshold
            {
                System_Status_Global&= ~DC_OVER_VOLT;               //Clear the bus overvoltage fault flag
                DCBusProtecion_M.OverCnt = 0;
            }
        }
        if((System_Status_Global&DC_UNDER_VOLT)==DC_UNDER_VOLT)      //For the bus undervoltage fault
        {
            if(ADC_M.DCVoltAver > DCBusProtecion_M.UnderThresh)  //Check whether it is higher than the bus undervoltage threshold
            {
                System_Status_Global&= ~DC_UNDER_VOLT;              //Clear the bus undervoltage fault flag
                DCBusProtecion_M.UnderCnt = 0;
            }
        }
    }

    //Re-check the faults
    if ( (System_Status_Global & (STARTUP_FAILURE_M |SPEED_ABNORMAL_M | MOTOR_OVER_CURR_SOFT_M | MOTOR_OVERLOAD_M\
             | DC_OVER_CURR_HARD_M | MOTOR_STALL_M |PHASELOSS_FALUT | DC_OVER_VOLT | DC_UNDER_VOLT)) == 0 )
    {
      RunningStatus_M = IDLE;                   //Set the IDLE status
    }
}

/******************************************************************************
* Function Name  : DCOvervolt_Protection
* Description    : Bus overvoltage protection
* Input          : Bus voltage to detect, pointer to the voltage protection structure
* Output         : None
* Return         : None
******************************************************************************/
void DCOvervolt_Protection(_iq24 DCVolt, DCbusProtection_Type *DCbusProtec)
{
    if (DCVolt >= DCbusProtec->OverThresh)
        DCbusProtec->OverCnt++;
    else
        DCbusProtec->OverCnt = 0;

    if(DCbusProtec->OverCnt > DCbusProtec->OverCntThresh)
    {
        Protection_SetFault(DC_OVER_VOLT);//Bus overvoltage
        DCBusProtecion_M.Overvolt = TRUE;
    }
}

/******************************************************************************
* Function Name  : DCUndervolt_Protection
* Description    : Bus undervoltage protection
* Input          : Bus voltage to detect, pointer to the voltage protection structure
* Output         : None
* Return         : None
******************************************************************************/
void DCUndervolt_Protection(_iq24 DCVolt, DCbusProtection_Type *DCbusProtec)
{
    if (DCVolt <= DCbusProtec->UnderThresh)
         DCbusProtec->UnderCnt++;
    else
        DCbusProtec->UnderCnt = 0;

    if(DCbusProtec->UnderCnt > DCbusProtec->UnderCntThresh)
    {
        Protection_SetFault(DC_UNDER_VOLT);//Bus undervoltage
        DCBusProtecion_M.Undervolt = TRUE;
    }
}
/******************************************************************************
* Function Name  : Phase_Ins_OverCurrent_Verify
* Description    : Instantaneous overcurrent fault verification
* Input          : Pointer to the three-phase stationary coordinate system current, pointer to the overcurrent verification structure
* Output         : Overcurrent verification result
* Return         : Overcurrent verification result
*****************************************************************************/
Truth_Verify_Type Phase_Ins_OverCurrent_Verify (Component_3S_Type *PhaseABC, InstPhaseOC_Type * InstOC)
{
    if ((_IQabs(PhaseABC->PhaseA) > InstOC->CurrThreshold)||\
        (_IQabs(PhaseABC->PhaseB) > InstOC->CurrThreshold)||\
        (_IQabs(PhaseABC->PhaseC) > InstOC->CurrThreshold))//Exceeds the overcurrent threshold continuously, set, otherwise clear the count
        InstOC->Cnt++;
    else
        InstOC->Cnt =0;

    //The current over-limit count exceeds the count threshold, set the fault
    if (InstOC->Cnt > InstOC->CntThreshold)
      return(TRUE);
    else
      return(FALSE);
}

/******************************************************************************
* Function Name  : Speed_Outrange_Verify
* Description    : Speed out of range verification
* Input          : Reference speed, actual speed, pointer to the observer status structure
* Output         : Overspeed and underspeed verification result
* Return         : None
******************************************************************************/
void Speed_Outrange_Verify(_iq24 Speedact, Observer_Status_Type *Status)
{
    //Overspeed verification
    if (Speedact > Status->OverspeedThresh)
    {
        Status->Overspeed_CNT++;
        if(Status->Overspeed_CNT > Status->OverspeedcntThresh)
            Status->Overspeed = TRUE;
    }
    else
    {
        Status->Overspeed_CNT = 0;
        Status->Overspeed = FALSE;
    }

    //Underspeed verification
    if (Speedact < Status->UnderspeedThresh)
    {
        Status->Underspeed_CNT++;
        if(Status->Underspeed_CNT > Status->UnderspeedcntThresh)
            Status->Underspeed = TRUE;
    }
    else
    {
        Status->Underspeed_CNT = 0;
        Status->Underspeed = FALSE;
    }

}

/******************************************************************************
* Function Name  : Speed_Reliable_Verify
* Description    : Speed reliability verification
* Input          : Pointer to the speed array parameters, array pointer, average speed, reliability verification threshold.
* Output         : Reliability verification result
* Return         : None
*******************************************************************************/
Truth_Verify_Type Speed_Reliable_Verify(BuffPara_Type *Buffpara,_iq24 *Buffer,_iq24 Averspeed,_iq24 Threshold)
{
    //Local variable definition
    uint8_t i;
    _iq24 Deviation;                  //Speed deviation
    _iq24 Deviation_Square =0;        //Speed variance
    _iq24 Deviation_Square_Threshold; //Allowed speed variance
    Truth_Verify_Type Reliable;

    //Speed variance calculation
    for (i=0 ; i < Buffpara->Length ; i++)
    {
        Deviation = *(Buffer + i) - Averspeed;
        Deviation_Square = Deviation_Square + _IQMPY_MACRO(Deviation , Deviation);

    }
    Deviation_Square = _IQMPY_MACRO(Deviation_Square,Buffpara->Inv1);

    //Allowed variance calculation
    Deviation_Square_Threshold = _IQMPY_MACRO(_IQMPY_MACRO(Averspeed,Averspeed),Threshold);

    //Speed reliability verification
    if(Deviation_Square >= Deviation_Square_Threshold)
        Reliable = FALSE;
    else
        Reliable = TRUE;

    return(Reliable);
}

/******************************************************************************
* Function Name  : Speed_Stable_Verify
* Description    : Speed stability verification, used during running
* Input          : Pointer to the observer status structure, actual speed
* Output         : Stability verification result
* Return         : None
******************************************************************************/
void Speed_Stable_Verify(Observer_Status_Type *Status,_iq24 Speedact)
{
//Continuous verification of the speed variance out of range
  if (_IQabs(Speedact) >= Status->UnstableSpeedThresh)
  {
      if(Status->Reliable == FALSE)//Speed variance out of range
      {
          Status->Unstable_CNT++;//Counter accumulation
        if (Status->Unstable_CNT >= Status->UnstableCntThresh)//Consecutive out-of-range count exceeds the defined value
        {
            Status->Unstable_CNT = 0;
            Status->Stable = FALSE;//Return abnormal
        }
        else;
      }
      else
      {
          Status->Unstable_CNT = 0;
          Status->Stable = TRUE;//Return normal
      }
  }
}
/******************************************************************************
* Function Name  : Motor_Stall_Verify2
* Description    : Motor stall verification, to prevent startup failure and stall during running
* Input          : Reference speed, feedback speed, current reference, pointer to the observer status structure
* Output         : Stall verification result
* Return         : None
*******************************************************************************/
void Motor_Stall_Verify2(_iq24 Speedref, _iq24 Speedact , _iq24 Iqref, Observer_Status_Type *Status)
{
    _iq24 Speedband;
    _iq24 SpeedErr;

    //Deviation width calculation
    Speedband = _IQMPY_MACRO(Speedref,Status->StallSpeedBand);

    //Speed deviation calculation
    SpeedErr = _IQabs(Speedref - Speedact);

    //Stall verification
    if (((SpeedErr >= Speedband)&&(Iqref >= Status->StallCurr))|| \
            ((_IQabs(Speedact) <= Status->StallSpeed)&&(Iqref >= Status->StallCurr)))
    {
        Status->Stall_CNT++;
        if(Status->Stall_CNT > Status->StallCntThresh)
            Status->Stall = TRUE;
    }
    else
    {
        Status->Stall_CNT = 0;
        Status->Stall = FALSE;
    }
}
/******************************************************************************
* Function Name  : Motor_Stall_Verify3
* Description    : Motor stall verification, to prevent startup failure and stall during running
* Input          : Reference speed, feedback speed, current reference, pointer to the observer status structure
* Output         : Stall verification result
* Return         : None
*******************************************************************************/
void Motor_Stall_Verify(_iq24 Speedref, _iq24 EalphaNew ,_iq24 EbetaNew,_iq24 Iqref, Observer_Status_Type *Status, _iq24 Angspeed)
{
    _iq24 Speedband;
    _iq24 SpeedErr;
    _iq24 EmfSquare;
    _iq24 EmfPeak;
    _iq24 EmfErr;

    //Deviation width calculation
    Speedband = _IQMPY_MACRO(_IQabs(Speedref),Status->StallSpeedBand);

    //Speed deviation calculation
    SpeedErr = _IQabs(Speedref - Angspeed);

    //Back EMF deviation calculation
    EmfSquare = _IQMPY_MACRO( EalphaNew, EalphaNew)+_IQMPY_MACRO( EbetaNew, EbetaNew);

    EmfPeak = _IQsqrt(EmfSquare);

    EmfErr = (_IQabs(Speedref) - EmfPeak);//Only verify the stall when the reference speed is greater than EmfPeak; adding ABS would also count the overshoot as a stall

    //Stall verification
    if ( ((SpeedErr >= Speedband)&&(_IQabs(Iqref)>= Status->StallCurr))||\
        ((_IQabs(Angspeed) <= Status->StallSpeed)&&(_IQabs(Iqref)>= Status->StallCurr))||\
       ((EmfErr >= Speedband)&&(_IQabs(Speedref) >=_IQ(0.15))))
    {
        Status->Stall_CNT++;
        if(Status->Stall_CNT > Status->StallCntThresh)
            Status->Stall = TRUE;
    }
    else
    {
        Status->Stall_CNT = 0;
        Status->Stall = FALSE;
    }
}

/******************************************************************************
* Function Name  : OverLoad_Verify
* Description    : Overload verification
* Input          : Pointer to the three-phase stationary coordinate system current, pointer to the overload verification structure, pointer to the current filter structure
* Output         : Overload verification result
* Return         : Overload verification result
*****************************************************************************/
Truth_Verify_Type OverLoad_Verify (Component_3S_Type *PhaseABC, OverLoad_Type * Overload, LPF_First_Order_Type * LPF)
{
    _iq24 Curr_Local;
    Curr_Local = _IQabs(PhaseABC->PhaseA) + _IQabs(PhaseABC->PhaseB) + _IQabs(PhaseABC->PhaseC);
    Curr_Local = _IQMPY_MACRO(Curr_Local,Overload->CalCoef);
    Overload->CurrCal = LPF_1st_Process(LPF, Curr_Local);

    if (Overload->CurrCal > Overload->CurrThresh)    //Exceeds the overload threshold continuously, the counter accumulates, otherwise the count is cleared
        Overload->Cnt++;
    else
        Overload->Cnt =0;

    //The current over-limit count exceeds the count threshold, set the fault
    if (Overload->Cnt > Overload->CntThresh)
      return(TRUE);
    else
      return(FALSE);
}

/******************************************************************************
* Function Name  : Phaseloss_Verify1R
* Description    : Phase loss verification
* Input          : Pointer to the three-phase stationary coordinate system current, pointer to the phase loss verification structure, SVPWM sector
* Output         : Overload verification result
* Return         : Overload verification result
*****************************************************************************/
Truth_Verify_Type Phaseloss_Verify1R(Component_3S_Type *PhaseABC,Phaseloss_Type *Phaseloss, uint8_t PWMsector)
{
    _iq24 CurrAabslocal;
    _iq24 CurrBabslocal;
    _iq24 CurrCabslocal;

    CurrAabslocal = _IQabs(PhaseABC->PhaseA);
    CurrBabslocal = _IQabs(PhaseABC->PhaseB);
    CurrCabslocal = _IQabs(PhaseABC->PhaseC);

    Phaseloss->CurrAabs = AverValue_Cal(CurrAabslocal, Phaseloss->CurrAabs, &Phaseloss->CurrBuffPara);
    Phaseloss->CurrBabs = AverValue_Cal(CurrBabslocal, Phaseloss->CurrBabs, &Phaseloss->CurrBuffPara);
    Phaseloss->CurrCabs = AverValue_Cal(CurrCabslocal, Phaseloss->CurrCabs, &Phaseloss->CurrBuffPara);

    //Two-phase loss and three-phase loss
    if((Phaseloss->CurrAabs<=Phaseloss->CurrThresh)&&(Phaseloss->CurrBabs<=Phaseloss->CurrThresh)&&(Phaseloss->CurrCabs<=Phaseloss->CurrThresh))
        Phaseloss->MultiphaseCnt++;
    else
        Phaseloss->MultiphaseCnt = 0;

    //Phase A loss judgement
    switch(PWMsector)
    {
        case 1:
        case 3:
        case 4:
        case 6:
            if(Phaseloss->CurrAabs <= Phaseloss->CurrThresh)
                Phaseloss->SiglephaseCntA++;
            else
                Phaseloss->SiglephaseCntA = 0;
            break;
        default:
            break;
    }

    //Phase B loss judgement
    switch(PWMsector)
    {
        case 2:
        case 3:
        case 5:
        case 6:
            if(Phaseloss->CurrBabs <= Phaseloss->CurrThresh)
                Phaseloss->SiglephaseCntB++;
            else
                Phaseloss->SiglephaseCntB = 0;
            break;
        default:
            break;
    }

    //Phase C loss judgement
    switch(PWMsector)
    {
        case 1:
        case 2:
        case 4:
        case 5:
            if(Phaseloss->CurrCabs <= Phaseloss->CurrThresh)
                Phaseloss->SiglephaseCntC++;
            else
                Phaseloss->SiglephaseCntC = 0;
            break;
        default:
            break;
    }

    //The over-limit count exceeds the count threshold, set the fault
    if ((Phaseloss->SiglephaseCntA > Phaseloss->CntThresh)||(Phaseloss->SiglephaseCntB > Phaseloss->CntThresh)\
            ||(Phaseloss->SiglephaseCntC > Phaseloss->CntThresh)||(Phaseloss->MultiphaseCnt > Phaseloss->CntThresh))
        return(TRUE);
    else
        return(FALSE);
}

/******************************************************************************
* Function Name  : Phaseloss_Verify2R3R
* Description    : Phase loss judgement
* Input          : Pointer to the three-phase stationary coordinate system current, pointer to the phase loss verification structure, pointer to the motor global state structure
* Output         : Overload verification result
* Return         : Overload verification result
*****************************************************************************/
Truth_Verify_Type Phaseloss_Verify2R3R (Phaseloss_Type *Phaseloss,Motor_Sructure_Type *MStruc)
{
    _iq24 CurrAabslocal;
    _iq24 CurrBabslocal;
    _iq24 CurrCabslocal;

    CurrAabslocal = _IQabs(MStruc->C3S.PhaseA);
    CurrBabslocal = _IQabs(MStruc->C3S.PhaseB);
    CurrCabslocal = _IQabs(MStruc->C3S.PhaseC);


    Phaseloss->CurrAabs = AverValue_Cal(CurrAabslocal, Phaseloss->CurrAabs, &Phaseloss->CurrBuffPara);
    Phaseloss->CurrBabs = AverValue_Cal(CurrBabslocal, Phaseloss->CurrBabs, &Phaseloss->CurrBuffPara);
    Phaseloss->CurrCabs = AverValue_Cal(CurrCabslocal, Phaseloss->CurrCabs, &Phaseloss->CurrBuffPara);

    if((_IQabs(MStruc->CRef2R.cd) > (Phaseloss->CurrThresh<<2))||(_IQabs(MStruc->CRef2R.cq) > (Phaseloss->CurrThresh<<2)))
    {
//        //Two-phase loss and three-phase loss
        if((Phaseloss->CurrAabs<=Phaseloss->CurrThresh)&&(Phaseloss->CurrBabs<=Phaseloss->CurrThresh)&&(Phaseloss->CurrCabs<=Phaseloss->CurrThresh))
            Phaseloss->MultiphaseCnt++;
        else
            Phaseloss->MultiphaseCnt = 0;

        //Phase A loss judgement
        if(Phaseloss->CurrAabs <= Phaseloss->CurrThresh)
            Phaseloss->SiglephaseCntA++;
        else
            Phaseloss->SiglephaseCntA = 0;

        //Phase B loss judgement
        if(Phaseloss->CurrBabs <= Phaseloss->CurrThresh)
            Phaseloss->SiglephaseCntB++;
        else
            Phaseloss->SiglephaseCntB = 0;

        //Phase C loss judgement
        if(Phaseloss->CurrCabs <= Phaseloss->CurrThresh)
            Phaseloss->SiglephaseCntC++;
        else
            Phaseloss->SiglephaseCntC = 0;
    }
    else
    {
         Phaseloss->MultiphaseCnt = 0;
         Phaseloss->SiglephaseCntA = 0;
         Phaseloss->SiglephaseCntB = 0;
         Phaseloss->SiglephaseCntC = 0;
    }

    //The over-limit count exceeds the count threshold, set the fault
    if ((Phaseloss->SiglephaseCntA > Phaseloss->CntThresh)||(Phaseloss->SiglephaseCntB > Phaseloss->CntThresh)\
            ||(Phaseloss->SiglephaseCntC > Phaseloss->CntThresh)||(Phaseloss->MultiphaseCnt > Phaseloss->CntThresh))
        return(TRUE);
    else
        return(FALSE);
}

