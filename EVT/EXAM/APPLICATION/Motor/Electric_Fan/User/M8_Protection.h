/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __M8_PROTECTION_H
#define __M8_PROTECTION_H

/* Includes -----------------------------------------------------------------*/
/* Exported types -----------------------------------------------------------*/

//Structure prototype of the instantaneous overcurrent protection
typedef struct
{
    _iq24 CurrThreshold;      //Overcurrent threshold (unit-A_per)
    uint16_t CntThreshold;    //Count threshold for satisfying the condition
    uint16_t Cnt;             //Count value for satisfying the condition
} InstPhaseOC_Type;

//Structure prototype of the bus voltage protection
typedef struct
{
    _iq24 OverThresh;               //Overvoltage threshold (unit-V_per)
    _iq24 UnderThresh;              //Undervoltage threshold (unit-V_per)

    uint16_t UnderCntThresh;        //Count threshold of the undervoltage protection
    uint16_t UnderCnt;              //Count value of the undervoltage protection
    uint16_t OverCntThresh;         //Count threshold of the overvoltage protection
    uint16_t OverCnt;               //Count value of the overvoltage protection

    Truth_Verify_Type Undervolt;    //Undervoltage flag
    Truth_Verify_Type Overvolt;     //Overvoltage flag
} DCbusProtection_Type;

//Structure prototype of the overload protection
typedef struct
{
    _iq24 CurrCal;         //Calculated current of the overload (unit-A_per)
    _iq24 CurrThresh;      //Overload threshold (unit-A_per)
    _iq24 CalCoef;         //Calculation coefficient
    uint16_t CntThresh;    //Count threshold for satisfying the condition
    uint16_t Cnt;          //Count value for satisfying the condition
} OverLoad_Type;

//Structure prototype of the phase loss protection
typedef struct
{
    _iq24 CurrThresh;                 //Phase loss current threshold (unit-A_per)
    _iq24 CurrAabs;                   //Absolute value of the phase A current (unit-A_per)
    _iq24 CurrBabs;                   //Absolute value of the phase B current (unit-A_per)
    _iq24 CurrCabs;                   //Absolute value of the phase C current (unit-A_per)
    BuffPara_Type CurrBuffPara;       //Parameters of the current average value calculation

    uint16_t CntThresh;               //Count threshold of the phase loss condition
    uint16_t SiglephaseCntA;          //Count value of the phase U loss condition
    uint16_t SiglephaseCntB;          //Count value of the phase V loss condition
    uint16_t SiglephaseCntC;          //Count value of the phase W loss condition
    uint16_t MultiphaseCnt;           //Count value of the multi-phase loss condition

    uint16_t DetectphaseSet;          //Setting value of the phase loss detection count
    uint16_t DetectphaseCnt;          //Phase loss detection count
    Truth_Verify_Type Detectphase;    //Phase loss detection flag
} Phaseloss_Type;
/* Exported constants -------------------------------------------------------*/
//Overcurrent protection parameter definition
#define INST_PHASEOC_THRE_M       _IQ(1.2)      //AC instantaneous overcurrent software protection threshold (unit-A_per)
#define INST_PHASEOC_CNT_THRE_M   (uint16_t)2   //Count threshold of the AC instantaneous overcurrent software protection

//Bus voltage protection parameter definition
#define DCBUS_OVER_VOLT_THRE    _IQ(1.2)        //Bus overvoltage threshold (unit-V_per) 90V
#define DCBUS_UNDER_VOLT_THRE   _IQ(0.8)        //Bus undervoltage threshold (unit-V_per) 37.8V
#define DCBUS_OVER_VOLT_CNT     (uint16_t)500   //Count threshold of the bus overvoltage protection
#define DCBUS_UNDER_VOLT_CNT    (uint16_t)500   //Count threshold of the bus undervoltage protection

//Phase loss protection parameter definition
#define PHASELOSS_DETECT_TIME    0.5            //Phase loss detection start time (unit-s)
#define PHASELOSS_CURR_THRE      _IQ(0.05)      //Phase loss current threshold (unit-A_per)
#define PHASELOSS_TIME_THRE      0.2            //Phase loss time threshold (unit-s)
#define PHASELOSS_CAL_FREQ       FREQ_SVPWM     //Phase loss calculation frequency (unit-Hz)
#define PHASELOSS_CAL_BUFFSIZE   16             //Phase loss calculation buffer depth
#define PHASELOSS_CNT_THRE       3              //Phase loss calculation count

/* Exported variables -------------------------------------------------------*/
extern InstPhaseOC_Type  InstPhaseOC_M;        //AC instantaneous overcurrent software protection
extern DCbusProtection_Type DCBusProtecion_M;  //Bus voltage protection
extern Phaseloss_Type Phaseloss_M;             //Phase loss protection structure
extern DCurrProcess_Type    DCurrProcess_M;    //D axis current processing structure

/* Exported macro -----------------------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
//Fault and protection related parameter definition
void Protection_and_Emergency_Define (void);

//Fault and protection related initialization
void Protection_and_Emergency_Init (void);

//Fault action definition
void Protection_SetFault(uint16_t Fault_type);

//Fault clear, performed by the reset key
void Protction_Fault_Reset_M(void);

//Software AC instantaneous overcurrent verification
Truth_Verify_Type Phase_Ins_OverCurrent_Verify (Component_3S_Type *PhaseABC, InstPhaseOC_Type * InstOC);

//Bus overvoltage protection
void DCOvervolt_Protection(_iq24 DCVolt, DCbusProtection_Type *DCbusProtec);

//Bus undervoltage protection
void DCUndervolt_Protection(_iq24 DCVolt, DCbusProtection_Type *DCbusProtec);

//Speed reliability verification
Truth_Verify_Type Speed_Reliable_Verify(BuffPara_Type *Buffpara,_iq24 *Buffer,_iq24 Averspeed,_iq24 Threshold);

//Speed stability verification
void Speed_Stable_Verify(Observer_Status_Type *Status,_iq24 Speedact);

//Stall verification
void Motor_Stall_Verify(_iq24 Speedref, _iq24 EalphaNew ,_iq24 EbetaNew,_iq24 Iqref, Observer_Status_Type *Status, _iq24 Angspeed);

//Overspeed and underspeed protection
void Speed_Outrange_Verify(_iq24 Speedact, Observer_Status_Type *Status);

//Phase loss verification
Truth_Verify_Type Phaseloss_Verify2R3R (Phaseloss_Type *Phaseloss,Motor_Sructure_Type *MStruc);

#endif
