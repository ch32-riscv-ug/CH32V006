/* Includes ------------------------------------------------------------------*/
#include "M0_Control_Library.h"

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private functions --------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Variables ----------------------------------------------------------------*/
HFVQI_Start_Type    HFVQI_StartStruc_M;         //Injected voltage pulse positioning structure
BuffPara_Type       HFVQIPosi_CBuffPara_M;      //Injected voltage pulse positioning observed current buffer parameters
_iq24   HFVQIPosi_CBuff_M[HFVQI_POSI_SECTOR_M]; //Injected voltage pulse positioning observed current buffer
/******************************************************************************/
/* Function Name  : HFVQI_Parameter_Define
/* Description : HFVQI initial positioning parameter initialization
/* Input:   None
/* Output : None
/* Return : None
/******************************************************************************/
void HFVQI_Parameter_Define (HFVQI_Start_Type *HFVQI_Start,SVPWM_2R_Type *SVPWM_Cal)
{
    //Injected pulse positioning parameter calculation
    HFVQI_Start->Vmag = HFVQI_POSI_VMAG_M;
    //Injected voltage limiting of the SVPWM in the positioning stage
    HFVQI_Start->Vmagmax = HFVQI_POSI_VMAGMAX_M;
    HFVQI_Start->Vmagmin = HFVQI_POSI_VMAGMIN_M;

    HFVQI_Start->TimeStep = HFVQI_POSI_STEP_M;
    HFVQI_Start->TimeEffect = HFVQI_POSI_PULSE_EFFECTIVE_M;

    HFVQI_Start->AngleInit = HFVQI_POSI_ANGLE_INIT_M;

    HFVQI_Start->AngleStep = _IQ(1.0)/HFVQI_POSI_SECTOR_M;
    HFVQI_Start->SectorSet = HFVQI_POSI_SECTOR_M;
    HFVQI_Start->Posi_Cali = _IQ(1.0)/(HFVQI_POSI_SECTOR_M*2);

    HFVQIPosi_CBuffPara_M.Length = HFVQI_POSI_SECTOR_M;

    HFVQI_Start->Phaseloss_Currthre = PHASELOSS_CURR_THRE;
    HFVQI_Start->Phaseloss_Cntthre = PHASELOSS_CNT_THRE;
}

/*******************************************************************************/
/* Function Name  : HFVQI_PosiSearch_Status_Init_M
/* Description : HFVQI initial positioning status initialization
/* Input:   None
/* Output : None
/* Return : None
/******************************************************************************/
void HFVQI_Status_Init_M (HFVQI_Start_Type *HFVQI_Start)
{
    //Injected pulse positioning parameter initialization
    HFVQI_Start->Counter = 0;
    HFVQI_Start->Position = 0;
    HFVQI_Start->SectorAct = 0;
    HFVQI_Start->SectorMax = 0;
    HFVQI_Start->Finish = FALSE;
    
    HFVQI_Start->AngleInit = HFVQI_POSI_ANGLE_INIT_M ;

    HFVQI_Start->Curr2R.cd = 0;
    HFVQI_Start->Curr2R.cq = 0;

    HFVQI_Start->VoltRef2RC.cd = 0;
    HFVQI_Start->VoltRef2RC.cq = 0;

    HFVQI_Start->Phaseloss_Cnt = 0;
}

