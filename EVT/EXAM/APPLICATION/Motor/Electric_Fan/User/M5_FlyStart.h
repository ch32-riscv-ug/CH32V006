/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __M5_FLYSTART_H
#define __M5_FLYSTART_H
/* Includes -----------------------------------------------------------------*/
/* Exported types -----------------------------------------------------------*/
/* Exported constants -------------------------------------------------------*/

#define FLYSTART_FREQ_M                  FREQ_SVPWM                 //windmill startup processing frequency (unit-Hz)
#define FLYSTART_PERIOD_M                _IQ(1.0)/FLYSTART_FREQ_M   //windmill startup calculation period (unit-1s)
#define FLYSTART_VERIFY_TIME_M           0.3                        //windmill startup judgement time (unit-s)
#define FLYSTART_STILL_VERIFY_TIME_M     0.25                       //standstill judgement time threshold (unit-s), less than windmill judgement time

#define FLYSTARRTPLL_BANDWIDTH_M         45                         //PLL bandwidth (unit-Hz)
#define FLYSTARTPLL_ZETA                 _IQ(2)                     //PLL damping ratio

#define FLYSTART_VERIFY_NUM_M            10                         //windmill observation filter judgement count, timebase is current loop processing period

#define FLYSTART_CURRLOOP_GAIN_M         2                          //gain of windmill current loop left/right shift

#define FLYSTART_SPEED_LPF_CF_M          200                        //windmill speed low-pass filter cutoff frequency (unit-Hz)
#define FLYSTART_SPEED_LPF_PERIOD_M      FLYSTART_PERIOD_M          //windmill speed low-pass filter calculation period (unit-s)
#define FLYSTART_SPEED_LPF_WcT_M         _IQmpyI32(_IQmpy(FLYSTART_SPEED_LPF_PERIOD_M,PI_2_Q24),FLYSTART_SPEED_LPF_CF_M)

#define FLYSTART_EMFLPF_CF_M             500                        //back-EMF low-pass filter cutoff frequency (unit-Hz)
#define FLYSTART_EMFLPF_PERIOD_M         FLYSTART_PERIOD_M          //back-EMF low-pass filter calculation period (unit-s)
#define FLYSTART_EMFLPF_WcT_M            _IQmpyI32(_IQmpy(FLYSTART_EMFLPF_PERIOD_M,PI_2_Q24),FLYSTART_EMFLPF_CF_M)

#define FLYSTART_STILL_EMFMIN_M          _IQ(0.0078125)             //standstill judgement back-EMF threshold (unit-V_per)0.005
#define FLYSTART_BEMF_THRESHOLD_M        _IQ(0.002)                 //windmill detection back-EMF hold threshold
#define FLYSTART_SPEED_BOUNDARY_M        _IQ(0.03)                  //high/low speed boundary threshold (unit-rad/s_per)0.05
#define FLYSTART_STILL_CURR_M            _IQ(0.03)                  //standstill judgement current threshold (unit-A_per)

#define FLYSTART_STILLTIME               0.15                       //standstill braking time (unit-s)
#define FLYSTART_BREAKTIME               0.3                        //low-speed braking time (unit-s)

#define FLYSTART_SPEED_MAX_M            _IQ(1.0)                    //windmill observed angular speed upper limit (unit-rad/s_per)
#define FLYSTART_SPEED_MIN_M            _IQ(-1.0)                   //windmill observed angular speed lower limit (unit-rad/s_per)

#define VQ_AMPLITUDE_EFFSET              10                         //back-EMF amplitude judgement valid count value
#define VQ_ZERO_EFFSET                   5                          //back-EMF zero-crossing judgement valid count value
#define VQ_ZERODETECTION_EFFSET          10                         //back-EMF zero-crossing detection valid count value
/* Exported variables -------------------------------------------------------*/
extern Flystart_type  Flystart_M;   //back-wind detection back-EMF structure
/* Exported macro -----------------------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
//windmill startup parameter initialization
void Flystart_Prameter_Define(Flystart_type *Flystart);

//windmill startup parameter calculation
void Flystart_Paramter_Cal (Flystart_type *Flystart,Motor_Base_Type *Basestruc);

//windmill startup status initialization
void Flystart_Status_Init(Flystart_type *Flystart,int8_t Direction);

//windmill processing
void Flystart_Process(Flystart_type *Flystart,int8_t Direction,Motor_Sructure_Type *MStruc,SVPWM_2R_Type *SVPWM_Cal,\
        Motor_SMO_Type *SMOM,ADCStruc_Type *ADCStruc,Motor_PID_Type *Mpid);

//windmill startup switching control
void Flystart_Switch(Flystart_type *Flystart,Rampctr_Type *SpeedRamp);

//windmill startup control loop switching
void Flystart_CtrLoop_Inital(Flystart_type *Flystart,Motor_PID_Type *Mpid,\
                   Motor_Sructure_Type *MStruc,Motor_SMO_Type *MSMO);

#endif
