/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef _MA_HFVQI_OBSERVER_H
#define _MA_HFVQI_OBSERVER_H

/* Includes -----------------------------------------------------------------*/
/* Exported types -----------------------------------------------------------*/
/* Exported constants -------------------------------------------------------*/
#define HFVQI_POSI_VMAG_M               _IQ(0.4) //Voltage magnitude of the pulse positioning (unit-V_per)
#define HFVQI_POSI_VMAGMAX_M            _IQ(0.60) //Max voltage magnitude of the pulse positioning (unit-V_per)
#define HFVQI_POSI_VMAGMIN_M            _IQ(0.10) //Min voltage magnitude of the pulse positioning (unit-V_per)

#if defined  TWELVE_SECTOR_POSITION
#define HFVQI_POSI_SECTOR_M             12      //Number of pulse positioning positions
#endif

#if defined SIX_SECTOR_POSITION
#define HFVQI_POSI_SECTOR_M             6       //Number of pulse positioning positions
#endif

#define HFVQI_POSI_ANGLE_INIT_M         _IQ(-0.083333)//Initial angle of pulse positioning: _IQ(-1.0)/12

#define HFVQI_POSI_STEP_M               30       //Switching step of the pulse positioning, based on the calculation interrupt time  10
#define HFVQI_POSI_PULSE_EFFECTIVE_M    10       //Pulse voltage effective time, the actual value is the defined value minus 1  3


/* Exported variables -------------------------------------------------------*/
extern HFVQI_Start_Type HFVQI_StartStruc_M;     //HFVQI observer start control structure
extern BuffPara_Type    HFVQIPosi_CBuffPara_M;  //Injected voltage pulse positioning observed current buffer parameters
extern _iq24    HFVQIPosi_CBuff_M[];            //Injected voltage pulse positioning observed current buffer

/* Exported macro -----------------------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
//HFVQI initial positioning parameter initialization
void HFVQI_Parameter_Define (HFVQI_Start_Type *HFVQI_Start,SVPWM_2R_Type *SVPWM_Cal);

//HFVQI initial positioning status initialization
void HFVQI_Status_Init_M (HFVQI_Start_Type *HFVQI_Start);

//HFVQI initial positioning sector buffer initialization
void HFVQI_Buffer_Clear(void);

//HFVQI initial positioning execution function
void HFVQI_PosiSearch_Process(HFVQI_Start_Type *HFVQI_Start,SVPWM_2R_Type *SVPWM_Cal,_iq24 DCVoltAver , int8_t Direction);
#endif
