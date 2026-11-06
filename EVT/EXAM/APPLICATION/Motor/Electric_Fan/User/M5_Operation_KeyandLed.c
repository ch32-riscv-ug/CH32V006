/* Includes -----------------------------------------------------------------*/
#include "M0_Control_Library.h"
/* Private typedef ----------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Variables ----------------------------------------------------------------*/

/******************************************************************************
* Function Name  : KEY_AND_LEDIO_Init
* Description    : key and LED IO configuration
* Input          : None
* Output         : None
* Return         : None
******************************************************************************/
void KeyandLED_IOInit(void)
{
    //GPIO initialization structure definition
    GPIO_InitTypeDef GPIO_InitStructure={0};

    //clock enable
    RCC_PB2PeriphClockCmd(RCC_PB2Periph_AFIO|RCC_PB2Periph_GPIOB|RCC_PB2Periph_GPIOC, ENABLE);

    //key IO configuration PB3:KEY3->RUN PC4:KEY4->SPEED
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

}

/******************************************************************************
* Function Name  : KeyandLED_Parameter_Define
* Description    : speed and LED parameter definition
* Input          : pointer to key manager structure, pointer to LED manager structure
* Output         : None
* Return         : None
******************************************************************************/
void KeyandLED_Parameter_Define(Key_Manager_Type *KeyM)
{
    KeyM->RUN.Effectivetime = (uint16_t)(SRKEY_EFFECTIVE_TIME*KEY_CHECK_FREQ);
    KeyM->RUN.Ineffectivetime = (uint16_t)(SRKEY_INEFFECTIVE_TIME*KEY_CHECK_FREQ);
    KeyM->RUN.Group = GPIOB;
    KeyM->RUN.Pin = GPIO_Pin_3;
    KeyM->RUN.KeyPress=PRESSUP;

    KeyM->Speedadjust.Effectivetime = (uint16_t)(SRKEY_EFFECTIVE_TIME*KEY_CHECK_FREQ);
    KeyM->Speedadjust.Ineffectivetime = (uint16_t)(SRKEY_INEFFECTIVE_TIME*KEY_CHECK_FREQ);
    KeyM->Speedadjust.Group = GPIOC;
    KeyM->Speedadjust.Pin = GPIO_Pin_4;
    KeyM->Speedadjust.KeyPress=PRESSUP;
}

/******************************************************************************
* Function Name  : KeyandLED_Status_Init
* Description    : speed and LED status initialization
* Input          : pointer to key manager structure, pointer to LED manager structure
* Output         : None
* Return         : None
******************************************************************************/
void KeyandLED_Status_Init(Key_Manager_Type *KeyM)
{
    KeyM->Speedadjust.Effectivecnt = 0;
    KeyM->Speedadjust.Ineffectivecnt = 0;
    KeyM->Speedadjust.Status = FALSE;

    KeyM->RUN.Effectivecnt = 0;
    KeyM->RUN.Ineffectivecnt = 0;
    KeyM->RUN.Status = FALSE;
}

/******************************************************************************
* Function Name  : Skeycheck_Recover
* Description    : recoverable key detection -- single key, short press
* Input          : pointer to recoverable key structure
* Output         : None
* Return         : None
******************************************************************************/
void Skeycheck_Recover(Keyrecover_Type *Keyrec)
{
    //key detection
    if(GPIO_ReadInputDataBit(Keyrec->Group, Keyrec->Pin) == 0)//active low
    {
        if(Keyrec->Effectivecnt <= U16_MAX)
            Keyrec->Effectivecnt++;

        Keyrec->Ineffectivecnt=0;
    }
    else
    {
        if(Keyrec->Ineffectivecnt <= U16_MAX)
            Keyrec->Ineffectivecnt++;

            Keyrec->Effectivecnt=0;
    }
    if((Keyrec->Effectivecnt >= Keyrec->Effectivetime)&&(Keyrec->KeyPress==PRESSUP))//valid and in released state
    {
        Keyrec->Status=TRUE;
        Keyrec->Effectivecnt=0;
        Keyrec->KeyPress=PRESSDOWN;
    }
    else if((Keyrec->Ineffectivecnt>=Keyrec->Ineffectivetime))
    {
        Keyrec->Effectivecnt=0;
        Keyrec->Ineffectivecnt=0;
        Keyrec->KeyPress=PRESSUP;
    }

}

/******************************************************************************
* Function Name  : KeyCheck_Process
* Description    : key detection and processing
* Input          : pointer to key manager structure, pointer to acceleration/deceleration control structure
* Output         : None
* Return         : None
******************************************************************************/
void KeyCheck_Process(Key_Manager_Type *KeyM, Rampctr_Type *Ramp_M)
{

        Skeycheck_Recover(&(KeyM->RUN));

        if(((RunningStatus_M == RUN)&&(Ramp_M->Decemark == FALSE))&&(KeyM->RUN.KeyPress==PRESSUP))//in run state, not during deceleration, and RUN key is not pressed
        {
            Skeycheck_Recover(&(KeyM->Speedadjust));
        }

    //startup processing
     if(KeyM->RUN.Status == TRUE)
     {
         switch(Operation_M.Checkmode)
         {
             case 0:
                 //startup in stop state or fault state
                 if(( (RunningStatus_M == IDLE)||((RunningStatus_M == FAULT)) )&&(Operation_M.Checkmode ==0))
                 {
                       RunningStatus_M = INIT;

                       Ramp_Set(Ramp_M);

                       Operation_M.Checkmode =1;
                 }

                 //restart during deceleration
                 if((Operation_M.Checkmode ==0)&&(Ramp_M->Decemark ==TRUE))
                 {
                       Ramp_Set(Ramp_M);

                       Ramp_M->Decemark = FALSE;
                       Operation_M.Checkmode = 1;

                 }
                break;
            case 1:
                if(Operation_M.Checkmode ==1)
                {
                // for direct stop use
                // RunningStatus_M=STOP;

                // for deceleration stop use
#if     (CLOSE_LOOP==SPEED_LOOP)
                    Ramp_M->Preramp = BREAKSPEED;
#elif   (CLOSE_LOOP==POWER_LOOP)
                    Ramp_M->Preramp = BREAKPOWER;
#endif
                    Ramp_M->Decemark =TRUE;

                    Operation_M.Checkmode =0;

                }
                break;

            default:
                break;
             }

         KeyM->RUN.Status = FALSE;
     }

    //acceleration/deceleration processing
     if((KeyM->Speedadjust.Status == TRUE) &&(Ramp_M->Decemark == FALSE))
     {
         Ramp_M->Point_adncnt++;

         if(Ramp_M->Point_adncnt > POINT_RANK)
             Ramp_M->Point_adncnt = 1;

         KeyM->Speedadjust.Status = FALSE;

         Ramp_Set(Ramp_M);//speed setting
     }

    //deceleration stop processing
     //if((Ramp_M->Startfinish ==TRUE)&&(Ramp_M->Decemark ==TRUE))
     if(Ramp_M->Decemark ==TRUE)
     {
#if     (CLOSE_LOOP==SPEED_LOOP)
    //    if(_IQabs(SMO_M.Cal_Struc.Angspeed_Aver1)<=FAST_DECE_SPEED)
        if(_IQabs(Ramp_M->Aftramp)<=FAST_DECE_SPEED)
         {
                 if(RunningStatus_M!=FAULT)
                     RunningStatus_M = STOP;
                 Ramp_M->Decemark = FALSE;
         }
#elif   (CLOSE_LOOP==POWER_LOOP)
         //if(Power_Cal.AngPower_Aver1<=FAST_DECE_POWER)
        if(_IQabs(Ramp_M->Aftramp)<=FAST_DECE_POWER)
         {
             if(RunningStatus_M!=FAULT)
                 RunningStatus_M = STOP;
             Ramp_M->Decemark = FALSE;
         }
#endif
     }
}

