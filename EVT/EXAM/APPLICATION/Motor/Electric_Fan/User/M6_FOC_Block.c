/* Includes -----------------------------------------------------------------*/
// #include "M0_type_and_Constant.h"
#include "M0_Control_Library.h"
/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private functions --------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Variables ----------------------------------------------------------------*/
/******************************************************************************
* Function Name  : FOC_Cal
* Description    : FOC calculation
* Input          : Pointer to the motor overall state structure, pointer to the SVPWM structure,
*                  pointer to the motor PID regulator structure, pointer to the ADC structure,
*                  SMO observer structure,
* Output         : None
* Return         : None
******************************************************************************/
__attribute__((section(".highcode")))
void FOC_Cal(Motor_Sructure_Type *MStruc,SVPWM_2R_Type *SVPWM_Cal,Motor_PID_Type *Mpid,\
             ADCStruc_Type *ADCStruc,Motor_SMO_Type *SMOM,int8_t Direction,uint8_t ObsLPF_Enable)
{
        //Clark: convert the AB currents into alpha and beta currents
        MStruc->C2S.Alpha=MStruc->C3S.PhaseA;
        MStruc->C2S.Beta=_IQMPY_MACRO( (MStruc->C3S.PhaseA\
                        + MStruc->C3S.PhaseB+MStruc->C3S.PhaseB) ,DIV_SQRT3_Q24);

        switch(RunningStatus_M)
        {
            case DIRCHECK:
            case START:

                //Park: current calculation of the two-phase rotating coordinate system
                MStruc->C2R_H.cd=_IQMPY_MACRO( MStruc->C2S.Alpha,MStruc->Cossin.Cos)\
                                        +_IQMPY_MACRO( MStruc->C2S.Beta,MStruc->Cossin.Sin);
                MStruc->C2R_H.cq=_IQMPY_MACRO( MStruc->C2S.Beta,MStruc->Cossin.Cos)\
                                        -_IQMPY_MACRO( MStruc->C2S.Alpha,MStruc->Cossin.Sin);

                //d and q axis current filtering
                MStruc->C2R_H.cd = LPF_1st_Process(&(MStruc->IdLPF1stH), MStruc->C2R_H.cd);
                MStruc->C2R_H.cq = LPF_1st_Process(&(MStruc->IqLPF1stH), MStruc->C2R_H.cq);

                //Feedback current assignment
                MStruc->C2R.cd = MStruc->C2R_H.cd;
                MStruc->C2R.cq = MStruc->C2R_H.cq;
            break;
            case RUN:
                MStruc->Cossin = SMOM->Cal_Struc.Triangle;

                //Park: current calculation of the two-phase rotating coordinate system
                MStruc->C2R_H.cd=_IQMPY_MACRO( MStruc->C2S.Alpha,MStruc->Cossin.Cos)\
                                        +_IQMPY_MACRO( MStruc->C2S.Beta,MStruc->Cossin.Sin);
                MStruc->C2R_H.cq=_IQMPY_MACRO( MStruc->C2S.Beta,MStruc->Cossin.Cos)\
                                        -_IQMPY_MACRO( MStruc->C2S.Alpha,MStruc->Cossin.Sin);

                if(ObsLPF_Enable==1)
                {
                    //d and q axis current filtering
                    MStruc->C2R_H.cd = LPF_1st_Process(&(MStruc->IdLPF1stH), MStruc->C2R_H.cd);
                    MStruc->C2R_H.cq = LPF_1st_Process(&(MStruc->IqLPF1stH), MStruc->C2R_H.cq);

                    MStruc->C2S_Obs.Alpha=_IQMPY_MACRO(MStruc->C2R_H.cd,MStruc->Cossin.Cos)\
                                                -_IQMPY_MACRO(MStruc->C2R_H.cq,MStruc->Cossin.Sin);
                    MStruc->C2S_Obs.Beta=_IQMPY_MACRO(MStruc->C2R_H.cd,MStruc->Cossin.Sin)\
                                                +_IQMPY_MACRO(MStruc->C2R_H.cq,MStruc->Cossin.Cos);
                }
                else
                {
                    MStruc->C2S_Obs.Alpha=MStruc->C2S.Alpha;
                    MStruc->C2S_Obs.Beta=MStruc->C2S.Beta;
                }
                                            
                //Feedback current assignment
                MStruc->C2R.cd = MStruc->C2R_H.cd;
                MStruc->C2R.cq = MStruc->C2R_H.cq;
            break;

            default:
                break;
        }

        if(SMOM->Cal_Struc.Inimark==TRUE)
        {
            SMO_EMF_Cal(MStruc,SMOM);
            SMO_Speed_Posi_Cal(SMOM,Direction);
        }

        //Current PI regulation
        MStruc->VRef2R.cd = PI_Calculaion_Delta (MStruc->CRef2R.cd, MStruc->C2R.cd, &(Mpid->dCurr));
        MStruc->VRef2R.cq = PI_Calculaion_Delta (MStruc->CRef2R.cq, MStruc->C2R.cq, &(Mpid->qCurr));

        //Bus voltage correction of the d and q axis reference voltages
        MStruc->VRef2RC.cd = _IQMPY_MACRO(MStruc->VRef2R.cd,ADCStruc->DCVoltcali);
        MStruc->VRef2RC.cq = _IQMPY_MACRO(MStruc->VRef2R.cq,ADCStruc->DCVoltcali);

        //Keep the total output below the rated motor voltage when the bus voltage is higher than the rated bus voltage
        _iq24 ModLim = _IQMPY_MACRO(MOTOR_VOLT_CAP_PU, ADCStruc->DCVoltcali);

        if(ModLim > SVPWM_Cal->Maxmodule)
            ModLim = SVPWM_Cal->Maxmodule;

        _iq24 ModLimSqure = _IQMPY_MACRO(ModLim, ModLim);

        //d axis priority limiting: keep Vd, the Vq upper limit = Sqrt_Table(ModLim^2-Vd^2), table lookup rounded down to keep inside the circle
        ComponentLimit_DPriority(&(MStruc->VRef2RC), ModLimSqure, ModLim, &(SVPWM_Cal->VoltLimMark));

        //anti-windup: when limiting, divide the applied voltage by the bus correction coefficient and write it back to the PI state, and clamp it into the PI own limit,
        if(SVPWM_Cal->VoltLimMark != 0)
        {
            Mpid->dCurr.Output_Prev = _IQMPY_MACRO(MStruc->VRef2RC.cd, ADCStruc->DCVoltAver);                              
            Mpid->qCurr.Output_Prev = _IQMPY_MACRO(MStruc->VRef2RC.cq, ADCStruc->DCVoltAver);                            
        }

        //Rev_Park: calculation of the alpha and beta axis reference voltages used by SVPWM
        MStruc->VRef2SC.Alpha=_IQMPY_MACRO(MStruc->VRef2RC.cd,MStruc->Cossin.Cos)\
                                    -_IQMPY_MACRO(MStruc->VRef2RC.cq,MStruc->Cossin.Sin);
        MStruc->VRef2SC.Beta=_IQMPY_MACRO(MStruc->VRef2RC.cd,MStruc->Cossin.Sin)\
                                    +_IQMPY_MACRO(MStruc->VRef2RC.cq,MStruc->Cossin.Cos);

        //The observer uses the alpha and beta axis reference voltages for calculation
        MStruc->VRef2S.Alpha = _IQMPY_MACRO(MStruc->VRef2SC.Alpha,ADCStruc->DCVoltAver);
        MStruc->VRef2S.Beta = _IQMPY_MACRO(MStruc->VRef2SC.Beta,ADCStruc->DCVoltAver);

       SVPWM_Calculation(SVPWM_Cal,&(MStruc->VRef2SC));
}
