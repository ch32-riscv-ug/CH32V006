/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __M3_SMO_OBSERVER_H
#define __M3_SMO_OBSERVER_H
/* Includes -----------------------------------------------------------------*/
/* Exported types -----------------------------------------------------------*/
//observer status structure prototype
typedef struct
{
    _iq24 OverspeedThresh;          //overspeed threshold
    _iq24 UnderspeedThresh;         //underspeed threshold
    _iq24 VarianceThresh;           //variance threshold
    _iq24 UnstableSpeedThresh;      //unstable detection start speed threshold

    _iq24 StallSpeedBand;           //stall detection speed difference threshold
    _iq24 StallSpeed;               //stall detection speed threshold
    _iq24 StallCurr;                //stall detection current threshold

    uint16_t Overspeed_CNT;         //overspeed detection count
    uint16_t OverspeedcntThresh;    //overspeed detection count threshold
    uint16_t Underspeed_CNT;        //underspeed detection count
    uint16_t UnderspeedcntThresh;   //underspeed detection count threshold

    uint16_t Unstable_CNT;          //unstable detection count
    uint16_t UnstableCntThresh;     //unstable count detection threshold
    uint16_t Stall_CNT;             //stall detection count
    uint16_t StallCntThresh;        //stall detection count threshold

    Truth_Verify_Type Overspeed;    //overspeed
    Truth_Verify_Type Underspeed;   //underspeed
    Truth_Verify_Type Reliable;     //reliable

    Truth_Verify_Type Stable;       //stable
    Truth_Verify_Type Stall;        //running stall

} Observer_Status_Type;

/* Exported constants -------------------------------------------------------*/
#define SMO_CAL_FREQ_M          FREQ_SVPWM                         //SMO observer calculation frequency (unit-Hz)
#define SMO_CAL_PERIOD_M        _IQ(1.0)/SMO_CAL_FREQ_M           //SMO observer calculation period (unit-1s)

#define SMO_CERROR1_EMF_M        _IQ(0.3)                          //sliding mode gain 1
#define SMO_CERROR2_EMF_M        _IQ(0.1)                          //sliding mode gain 2
#define SMO_MAXSMCERROR          _IQ(0.1)                          //error boundary

#define SMO_MAX_EMF_M           _IQ(2.0)                           //observed back-EMF positive limit
#define SMO_MIN_EMF_M           _IQ(-2.0)                          //observed back-EMF negative limit value

#define SMO_MAX_CURR_M          _IQ(2.0)                           //observed current positive limit value
#define SMO_MIN_CURR_M          _IQ(-2.0)                          //observed current negative limit value

#define PLL_ZETA                 _IQ(1.0)                           //PLL damping ratio
#define PLL_BAND                 45                                 //PLL bandwidth (unit-Hz)
#define PLL_SPEED_DLIM          _IQ(0.15)                          //speed suppression upper limit (unit-rad/s_per)
#define PLL_SPEED_DLIM1         _IQ(0.5)                           //speed suppression upper limit 1 (unit-rad/s_per)
#define SMO_SPEED_MAX_M        _IQ(1.5)                            //SMO observed angular speed upper limit (unit-rad/s_per)
#define SMO_SPEED_MIN_M        _IQ(0.003)                          //SMO observed angular speed lower limit (unit-rad/s_per)

#define SMO_SPEED_BUFFERSIZE_M    32                               //observed speed average calculation array length

#define SMO_SETTING_TIME_M      0.05                                //observer settling time (unit-s)

#define OBS_LPF_EN                0                                 //observer current filter enable

//observer speed filter parameters
#define SMO_SPEEDLPF_CF_M               50                         //observed speed low-pass filter cutoff frequency (unit-Hz)
#define SMO_SPEEDLPF_PERIOD_M    (_IQ(1.0)/SPEED_LOOP_CAL_FREQ)    //observed speed low-pass filter calculation period (unit-s)
#define SMO_SPEEDLPF_WcT_M       _IQmpyI32(_IQmpy(SMO_SPEEDLPF_PERIOD_M,PI_2_Q24),SMO_SPEEDLPF_CF_M)

#define SMO_OVERSPEED_THRESH_M          _IQ(1.5)                   //SMO observed speed upper limit threshold (unit-rad/s_per)
#define SMO_UNDERSPEED_THRESH_M         _IQ(0.13)                  //SMO observed speed lower limit threshold (unit-rad/s_per)
#define SMO_OVERSPEED_CNT_THRESH_M      7000                       //SMO observed speed upper limit detection count threshold
#define SMO_UNDERSPEED_CNT_THRESH_M     7000                       //SMO observed speed lower limit detection count threshold

#define SMO_SPEED_VARIANCE_THRESH_M     _IQ(0.2)                   //SMO observed speed reliability judgement variance threshold

#define SMO_UNSTABLE_CNT_THRESH_M       300                        //SMO observed speed stability judgement count threshold
#define SMO_UNSTABLE_SPEED_THRESH_M     _IQ(0.02)                  //SMO observed speed stability judgement start speed threshold

#define SMO_STALLBAND_M         _IQ(0.5)                           //SMO observed stall protection bandwidth (unit-PU,0-1)
#define SMO_STALLSPEED_M        _IQ(0.1)                           //SMO observed stall protection speed (unit-rad/s_per)
#define SMO_STALLCURR_M         _IQMPY_MACRO(_IQ(0.9),SPEED_REGULATOR_ULIM_M)//SMO observed stall detection current (unit-A_per)

#define SMO_STALL_TIME_THRESH_M     3                              //stall detection threshold (unit-s)

/* Exported variables -------------------------------------------------------*/
extern Motor_SMO_Type SMO_M;                          //SMO observer structure
extern Observer_Status_Type    SMO_Status_M;           //SMO observer status structure
extern _iq24 SMO_SpeedBuffer_M[SMO_SPEED_BUFFERSIZE_M];//SMO observer observed speed average calculation array

/* Exported macro -----------------------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
//SMO observer status initialization
void SMO_Status_Init (Motor_SMO_Type *MSMO,Observer_Status_Type *SMOStatus,int8_t Direction);

//SMO observer parameter definition
void SMO_Parameter_Define (Motor_SMO_Type *MSMO,Observer_Status_Type *SMOStatus);

//SMO observer parameter calculation
void SMO_Paramter_Cal (Motor_Base_Type *Basestruc,Motor_SMO_Type *SMOM);

//SMO observer back-EMF calculation
void SMO_EMF_Cal(Motor_Sructure_Type *MStruc,Motor_SMO_Type *SMOM);

//SMO observer speed and angle calculation
void SMO_Speed_Posi_Cal(Motor_SMO_Type *SMOM,int8_t Direction);

//phase-locked loop with known angle error
_iq24 Speed_PLL_Cal(Speed_PLL_Type *SpeedPLL, _iq24 Theta_Err,int8_t Direction,_iq24 SpeedRef);
#endif
