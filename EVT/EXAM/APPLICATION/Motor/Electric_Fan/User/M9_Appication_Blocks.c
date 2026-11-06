/* Includes -----------------------------------------------------------------*/
#include "M0_Control_Library.h"

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private functions --------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/

/* Variables ----------------------------------------------------------------*/
//Definition of the sine table
const _iq24 Sin_Table_1024[256] = SIN_TABLE_1024;
extern const _iq24 Magnitude_table[256];
void TrigonoMetric_Function(Struct_Cossin_Type *Cossin,_iq24 Angle_PU);
const _iq24 Sqrt_Table_Q24[1024] = SQRT_TABLE_Q24;
/******************************************************************************
* Function Name  : PID_Calculaion_Delta
* Description    : Incremental PID calculation
* Input          : Reference, feedback, PID regulator structure
* Output         : PID calculation result
* Return         : PID calculation result
******************************************************************************/
_iq24 PID_Calculaion_Delta(_iq24 hReference, _iq24 hFeedback, PID_Structure_Type *PID_Struct)
{
    PID_Struct->Err_New = hReference - hFeedback;//Latest error calculation

    PID_Struct->Differential= _IQMPY_MACRO(PID_Struct->Kd,(PID_Struct->Err_New - 2*PID_Struct->Err_Prev1 + PID_Struct->Err_Prev2))*1;

    //PID incremental calculation
    PID_Struct->Deltaoutput =_IQMPY_MACRO(PID_Struct->Kp,(PID_Struct->Err_New - PID_Struct->Err_Prev1))
                      +_IQMPY_MACRO(PID_Struct->Ki,PID_Struct->Err_New)\
                      +PID_Struct->Differential;

    if(PID_Struct->Deltaoutput >= PID_Struct->Deltalim)
    {
        PID_Struct->Deltaoutput = PID_Struct->Deltalim;
    }
    else if (PID_Struct->Deltaoutput <= (-PID_Struct->Deltalim))
    {
        PID_Struct->Deltaoutput=(-PID_Struct->Deltalim);
    }
    PID_Struct->Output_New = PID_Struct->Output_Prev + PID_Struct->Deltaoutput;//Latest PID output calculation

    //PID output limiting
    if(PID_Struct->Output_New > PID_Struct->Upper_Lim)
        PID_Struct->Output_New = PID_Struct->Upper_Lim;
    else if (PID_Struct->Output_New < PID_Struct->Lower_Lim)
        PID_Struct->Output_New = PID_Struct->Lower_Lim;
    else;

    //Transfer of the error and PID regulator output values
    PID_Struct->Err_Prev2 = PID_Struct->Err_Prev1;
    PID_Struct->Err_Prev1 = PID_Struct->Err_New;

    PID_Struct->Output_Prev = PID_Struct->Output_New;

    return(PID_Struct->Output_New);
}
/******************************************************************************
* Function Name  : PI_Calculaion_Delta
* Description    : Incremental PI calculation
* Input          : Reference, feedback, PID regulator structure
* Output         : PI calculation result
* Return         : PI calculation result
******************************************************************************/
__attribute__((section(".highcode")))
_iq24 PI_Calculaion_Delta(_iq24 hReference, _iq24 hFeedback, PID_Structure_Type *PID_Struct)
{
    PID_Struct->Err_New = hReference - hFeedback;//Latest error calculation

    //PI incremental calculation
    PID_Struct->Deltaoutput =_IQMPY_MACRO(PID_Struct->Kp,(PID_Struct->Err_New - PID_Struct->Err_Prev1))
                      +_IQMPY_MACRO(PID_Struct->Ki,PID_Struct->Err_New);

    if(PID_Struct->Deltaoutput >= PID_Struct->Deltalim)
    {
        PID_Struct->Deltaoutput = PID_Struct->Deltalim;
    }
    else if (PID_Struct->Deltaoutput <= (-PID_Struct->Deltalim))
    {
        PID_Struct->Deltaoutput=(-PID_Struct->Deltalim);
    }

    PID_Struct->Output_New = PID_Struct->Output_Prev + PID_Struct->Deltaoutput;//Latest PI output calculation

    //PI output limiting
    if(PID_Struct->Output_New > PID_Struct->Upper_Lim)
        PID_Struct->Output_New = PID_Struct->Upper_Lim;
    else if (PID_Struct->Output_New < PID_Struct->Lower_Lim)
        PID_Struct->Output_New = PID_Struct->Lower_Lim;
    else;

    //Transfer of the error and PI regulator output values
    PID_Struct->Err_Prev2 = PID_Struct->Err_Prev1;
    PID_Struct->Err_Prev1 = PID_Struct->Err_New;

    PID_Struct->Output_Prev = PID_Struct->Output_New;

    return(PID_Struct->Output_New);
}
/******************************************************************************
* Function Name  : TrigonoMetric_Function
* Description    : Angle cosine and sine calculation
* Input          : Angle (0,1)
* Output         : Cosine and sine calculation result (IQ24)
* Return         : Cosine and sine calculation result.
******************************************************************************/
void TrigonoMetric_Function(Struct_Cossin_Type *Cossin,_iq24 Angle_PU)
{
    uint16_t Angle_Num;
    uint8_t Angle_Quard;
    Angle_Num = (uint16_t)((Angle_PU>>14)&0x000003FF);
    Angle_Quard = Angle_Num/256;
    switch(Angle_Quard)
    {
        case 0:
            Cossin->Sin = Sin_Table_1024[Angle_Num];
            Cossin->Cos = Sin_Table_1024[255 - Angle_Num];
            break;
        case 1:
            Cossin->Sin = Sin_Table_1024[511-Angle_Num];
            Cossin->Cos = -Sin_Table_1024[Angle_Num-256];
            break;
        case 2:
            Cossin->Sin = -Sin_Table_1024[Angle_Num - 512];
            Cossin->Cos = -Sin_Table_1024[767-Angle_Num];
            break;
        case 3:
            Cossin->Sin = -Sin_Table_1024[1023 - Angle_Num];
            Cossin->Cos = Sin_Table_1024[Angle_Num - 768];
            break;
    }

}
/******************************************************************************
* Function Name  : Clark_TransForm
* Description    : Clark transform
* Input          : Three-phase stationary coordinate system components
* Output         : Two-phase stationary coordinate system components
* Return         : Two-phase stationary coordinate system components
******************************************************************************/
Component_2S_Type Clark_TransForm(Component_3S_Type *Input3s)
{
    Component_2S_Type  Output_Component_2S;
    Output_Component_2S.Alpha=Input3s->PhaseA;
    Output_Component_2S.Beta=_IQMPY_MACRO( (Input3s->PhaseA\
                    + Input3s->PhaseB+Input3s->PhaseB) ,DIV_SQRT3_Q24);
    return(Output_Component_2S);
}

/******************************************************************************
* Function Name  : Park_TransForm
* Description    : Park transform
* Input          : Two-phase stationary coordinate system components, angle
* Output         : Two-phase rotating coordinate system components
* Return         : Two-phase rotating coordinate system components
******************************************************************************/
Component_2R_Type Park_TransForm(Component_2S_Type *Input2s,Struct_Cossin_Type *Struct_Cossin)
{
    Component_2R_Type Output_Component_2R;
    Output_Component_2R.cd=_IQMPY_MACRO(Input2s->Alpha,Struct_Cossin->Cos)\
                            +_IQMPY_MACRO(Input2s->Beta,Struct_Cossin->Sin);
    Output_Component_2R.cq=_IQMPY_MACRO(Input2s->Beta,Struct_Cossin->Cos)\
                            -_IQMPY_MACRO(Input2s->Alpha,Struct_Cossin->Sin);
    return(Output_Component_2R);
}

/******************************************************************************
* Function Name  : Rev_Park
* Description    : Inverse Park transform,
* Input          : Two-phase rotating coordinate system components, angle
* Output         : Two-phase stationary coordinate system components
* Return         : Two-phase stationary coordinate system components
******************************************************************************/
Component_2S_Type Rev_Park_TransForm(Component_2R_Type *Input2r,Struct_Cossin_Type *Struct_Cossin)
{
    Component_2S_Type Output_Component_2S;
    Output_Component_2S.Alpha=_IQMPY_MACRO(Input2r->cd,Struct_Cossin->Cos)\
                                -_IQMPY_MACRO(Input2r->cq,Struct_Cossin->Sin);
    Output_Component_2S.Beta=_IQMPY_MACRO(Input2r->cd,Struct_Cossin->Sin)\
                                +_IQMPY_MACRO(Input2r->cq,Struct_Cossin->Cos);
    return(Output_Component_2S);
}
/******************************************************************************
* Function Name  : ComponentLimit_2Ror2S
* Description    : Limiting calculation of the two-phase orthogonal coordinate system components, table lookup method
* Input          : Value before limiting
* Output         : Value after limiting
* Return         : Value after limiting
/******************************************************************************/
void ComponentLimit_2Ror2S(Component_2R_Type *Input,_iq24 Maxsquare,uint8_t *LimMark)
{
    uint16_t syntex;
    _iq24 Magnitude;
    Magnitude =_IQMPY_MACRO(Input->cd,Input->cd) + _IQMPY_MACRO(Input->cq,Input->cq);

    if(Magnitude > (Maxsquare+167772))//Add a bandwidth limit (0.01) to prevent frequent entry
    {
       syntex = (uint16_t)(((Magnitude>>17) - (Maxsquare>>17))&0x000003FF);
       if((syntex>=1)&&(syntex<=256))//Add the coefficient limit to prevent the effect of 0
       {
          Input->cd = _IQMPY_MACRO(Input->cd,Magnitude_table[ syntex-1]);
          Input->cq = _IQMPY_MACRO(Input->cq,Magnitude_table[ syntex-1]);
       }
       *LimMark = 1;
    }
    else
    {
       *LimMark = 0;
    }
}
static inline _iq24 Sqrt_Table(_iq24 s)
{
    if(s < 0) s = 0;
    if(s > (_iq24)0x01FFFFFF) s = (_iq24)0x01FFFFFF;   //Upper limit 2^25-1 (per unit < 2)
    return Sqrt_Table_Q24[(uint32_t)s >> 15];
}
/******************************************************************************
* Function Name  : ComponentLimit_DPriority
* Description    : d axis priority voltage limiting: keep Vd, Vq upper limit = sqrt(modulation ratio^2 - Vd^2)
* Input          : Value before limiting, limiting value squared, limiting value, limiting flag
* Output         : Value after limiting
* Return         : None
******************************************************************************/
void ComponentLimit_DPriority(Component_2R_Type *Input, _iq24 Maxsquare, _iq24 Maxmodule, uint8_t *LimMark)
{
    _iq24 Vd2 = _IQMPY_MACRO(Input->cd, Input->cd);

    if(Vd2 >= Maxsquare)                     //Vd itself reaches the limit: keep Vd, clear Vq
    {
        Input->cd = (Input->cd >= 0) ? Maxmodule : -Maxmodule;
        Input->cq = 0;
        *LimMark = 1;
    }
    else                                     //Get the Vq upper limit by table lookup, round down to keep inside the circle
    {
        _iq24 Vqmax = Sqrt_Table(Maxsquare - Vd2);

        if(Input->cq > Vqmax)        
        { 
            Input->cq =  Vqmax; 
            *LimMark = 1; 
        }
        else if(Input->cq < -Vqmax)  { Input->cq = -Vqmax; *LimMark = 1; }
        else                         *LimMark = 0;
    }
}
/******************************************************************************
* Function Name  : Round_Circle_Limitation
* Description    : Angle limited to the range 0-2PI
* Input          : Angle input
* Output         : Angle after limiting
* Return         : Angle after limiting
******************************************************************************/
_iq24 Round_Circle_Limitation_Rad(_iq24 Input)
{
    _iq24 Output;
    Output = Input;
    while(Output >= PI_2_Q24)
        Output = Output - PI_2_Q24;
    while(Output < 0)
        Output = Output + PI_2_Q24;
    return(Output);
}
/******************************************************************************
* Function Name  : Round_Circle_Limitation_PU
* Description    : Angle limited to the range 0-1
* Input          : Angle
* Output         : Angle after limiting
* Return         : Angle after limiting.
******************************************************************************/
_iq24 Round_Circle_Limitation_PU(_iq24 Input)
{
    _iq24 Output;
    Output = Input;
    while(Output >= UNIT_Q24)
        Output = Output - UNIT_Q24;
    while(Output < 0)
        Output = Output + UNIT_Q24;
    return(Output);
}
/******************************************************************************
* Function Name  : LPF_1st_Process
* Description    : First-order low-pass filter
* Input          : Value to be filtered, pointer to the filter parameter structure
* Output         : Filtered value
* Return         : Filtered value
******************************************************************************/
__attribute__((section(".highcode")))
_iq24 LPF_1st_Process(LPF_First_Order_Type *LPF_1st, _iq24 input)
{
    LPF_1st->Out_New = _IQMPY_MACRO(LPF_1st->Input_Coef,input)+\
                       _IQMPY_MACRO(LPF_1st->Output_Coef,LPF_1st->Out_Pre);
    LPF_1st->Out_Pre =  LPF_1st->Out_New;
    return( LPF_1st->Out_New);
}
/******************************************************************************
* Function Name  : HPF_1st_Process
* Description    : First-order high-pass filter
* Input          : Value to be filtered, pointer to the filter parameter structure
* Output         : Filtered value
* Return         : Filtered value
******************************************************************************/
_iq24 HPF_1st_Process(HPF_First_Order_Type *HPF_1st, _iq24 input)
{
    HPF_1st->Out_New = _IQMPY_MACRO(HPF_1st->Input_Coef,(input- HPF_1st->In_Pre))+\
                       _IQMPY_MACRO(HPF_1st->Output_Coef,HPF_1st->Out_Pre);

    HPF_1st->In_Pre =  input;
    HPF_1st->Out_Pre =  HPF_1st->Out_New;

    return( HPF_1st->Out_New);
}
/******************************************************************************
* Function Name  : Speedband_Avoid
* Description    : Avoidance of the speed reference band
* Input          : Speed reference input, avoidance band, avoidance bandwidth
* Output         : Speed reference after avoidance
* Return         : Speed reference after avoidance
******************************************************************************/
_iq24 Speedband_Avoid(_iq24 Speedinput,_iq24 Band,_iq24 Bandwidth)
{
    _iq24 Speedoutput;

    if(Speedinput >= 0)
    {
        if ((Speedinput >= Band) && (Speedinput < (Band + Bandwidth)))
            Speedoutput = Band + Bandwidth;
        else if ((Speedinput < Band) && (Speedinput >= (Band - Bandwidth)))
            Speedoutput = Band - Bandwidth;
        else
            Speedoutput = Speedinput;
    }
    else
    {
        if ((Speedinput >= -Band) && (Speedinput < -(Band - Bandwidth)))
            Speedoutput = -(Band - Bandwidth);
        else if ((Speedinput < -Band) && (Speedinput >= -(Band + Bandwidth)))
            Speedoutput = -(Band + Bandwidth);
        else
            Speedoutput = Speedinput;
    }
    return(Speedoutput);
}


/******************************************************************************
* Function Name  : Dcurr_Process_Init
* Description    : Initialization of the d axis current processing structure
* Input          : Pointer to the d axis current processing structure, target value, field weakening speed, current change step
* Output         : None
* Return         : None
******************************************************************************/
void Dcurr_Process_Init(DCurrProcess_Type * DCurrProcess,uint8_t Stage)
{
    if(Stage ==1)
    {
        DCurrProcess->Target = DCurrProcess->Target1;
        DCurrProcess->AdjustStep = DCurrProcess->AdjustStep1;
    }
    else if(Stage ==2)
    {
        DCurrProcess->Target = DCurrProcess->Target2;
        DCurrProcess->AdjustStep = DCurrProcess->AdjustStep2;
    }

    DCurrProcess->Stage = Stage;
}

/******************************************************************************
* Function Name  : Dcurr_Process
* Description    : D axis current processing
* Input          : Pointer to the d axis current processing structure, speed reference, current d axis current reference
* Output         : Real-time d axis current reference
* Return         : Real-time d axis current reference
******************************************************************************/
_iq24 Dcurr_Process(DCurrProcess_Type * DCurrProcess, _iq24 Speedact,_iq24 DCurrRefact)
{
    _iq24 DCurrRef;

    DCurrRef = DCurrRefact;

    //d axis current switching processing
    if(_IQabs(Speedact) > DCurrProcess->Weakpoint)//overspeed field weakening for sudden load reduction
    {
        DCurrRef = _IQ(-0.5);
    }
    else //recovery from current closed loop to VC control
    {
        if( DCurrRef > DCurrProcess->Target)
            DCurrRef = DCurrRef - DCurrProcess->AdjustStep;
        else if(DCurrRef  < DCurrProcess->Target)
            DCurrRef = DCurrRef + DCurrProcess->AdjustStep;
        else
            DCurrRef  = DCurrProcess->Target;
    }
    return(DCurrRef);
}
/******************************************************************************
* Function Name : AverValue_Cal
* Description   : Average value calculation
* Input         : Real-time value, array address, array parameters
* Output        : Average value
* Return        : Average value
******************************************************************************/
_iq24 AverValue_Cal(_iq24 Input,_iq24 Output, BuffPara_Type *Buffpara)
{
    //Local variable definition
    _iq24 Aver =0;

    //Average value calculation
    Aver = _IQMPY_MACRO(Output,Buffpara->Inv2)+_IQMPY_MACRO(Input,Buffpara->Inv1);

    return(Aver);
}


/******************************************************************************
* Function Name  : Buffer_Clear
* Description    : Zeroing of the _iq24 array
* Input          : Pointer to the array, pointer to the array parameter structure
* Output         : none
* Return         : none.
*******************************************************************************/
void Buffer_Clear(_iq24 *Buffer, BuffPara_Type *BuffPara)
{
    uint8_t BufferIndex;
    for(BufferIndex = 0; BufferIndex < BuffPara->Length; BufferIndex++)
    {
        *(Buffer + BufferIndex) = _IQ(0.0);
    }
    BuffPara->Index=0;
}

/******************************************************************************
* Function Name  : Buffer_Update
* Description    : Cyclic update of the _iq24 array
* Input          : Pointer to the array, pointer to the array parameter structure, update value
* Output         : none
* Return         : none.
*******************************************************************************/
void Buffer_Update(_iq24 *Buffer, BuffPara_Type *BuffPara,_iq24 Updatevalue)
{
    BuffPara->Index++;

    if (BuffPara->Index == BuffPara->Length)
        BuffPara->Index = 0;

    *(Buffer + BuffPara->Index) = Updatevalue;
}

/******************************************************************************
* Function Name  : Buffer_Set
* Description    : Set the _iq24 array to a fixed value
* Input          : Pointer to the array, pointer to the array parameter structure, set value
* Output         : none
* Return         : none.
*******************************************************************************/
void Buffer_Set(_iq24 *Buffer, BuffPara_Type *BuffPara,_iq24 Setvalue)
{
    uint8_t BufferIndex;
    for(BufferIndex = 0; BufferIndex < BuffPara->Length; BufferIndex++)
    {
        *(Buffer + BufferIndex) = Setvalue;
    }
    BuffPara->Index=0;
}
/******************************************************************************
* Function Name : IQ24divider
* Description   : Simple IQ24 division (range -128~127, precision 12bit)
* Input         : Dividend, divisor
* Output        : Quotient
* Return        : Quotient
******************************************************************************/
_iq24 IQ24divider(_iq24 dividendin, _iq24 divisorin)
{
    uint8_t sign_dividend =0;
    uint8_t sign_divisor =0;
    uint32_t dividend;
    uint32_t divisor;
    uint32_t quotient1;
    uint32_t quotient2;
    int32_t quotient3;

    if(dividendin < 0)
    {
        sign_dividend = 1;
        dividend = (uint32_t)(-dividendin);
    }
    else
        dividend = (uint32_t)dividendin;

    if(divisorin < 0)
    {
        sign_divisor = 1;
        divisor = (uint32_t)(-divisorin);
    }
    else
        divisor = (uint32_t)divisorin;

    quotient1 =  (dividend&0xFFFFF000)/(divisor>>12);
    quotient2 = (quotient1<<12);

    if(sign_dividend != sign_divisor)
        quotient3 = (-quotient2);
    else
        quotient3 = quotient2;

    return(quotient3);
}
