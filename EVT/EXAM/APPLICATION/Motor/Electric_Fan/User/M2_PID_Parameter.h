/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __M2_MOTOR_PID_H
#define __M2_MOTOR_PID_H

/* Includes -----------------------------------------------------------------*/
/* Exported types -----------------------------------------------------------*/
typedef struct
{
    _iq24 AngPower_Inst;                 //observed power real-time value
    _iq24 AngPower_Aver1;                //observed power average value
    PID_Structure_Type Power;            //power regulator structure
    LPF_First_Order_Type PowerLPF1st;    //power low-pass filter structure
} Motor_Power_Type;
/* Exported constants -------------------------------------------------------*/
//power regulator parameters
#define POWER_REGULATOR_FREQ_M    SPEED_LOOP_CAL_FREQ  //power regulator execution frequency (unit-Hz)
#define POWER_REGULATOR_PERIOD_M  (_IQ(1.0)/POWER_REGULATOR_FREQ_M)//power regulator execution period (unit-s)
#define POWER_REGULATOR_KP_M      _IQ(0.128)           //power regulator proportional coefficient (actual value)
#define POWER_REGULATOR_KI_M      _IQ(0.6)             //power regulator integral coefficient (actual value)
#define POWER_REGULATOR_ULIM_M    _IQ(1.0)             //power regulator output upper limit (unit-A_per)
#define POWER_REGULATOR_LLIM_M    _IQ(0.05)            //power regulator output lower limit (unit-A_per)
#define POWER_REGULATOR_DLIM_M    _IQ(0.2)             //power regulator single output limit (unit-A_per)

//power low-pass filtering
#define POWERLPF_CF_M        50                              //power low-pass filter cutoff frequency (unit-Hz)
#define POWERLPF_PERIOD_M    (_IQ(1.0)/SPEED_LOOP_CAL_FREQ)  //power low-pass filter calculation period (unit-s)
#define POWERLPF_WcT_M       _IQmpyI32(_IQmpy(POWERLPF_PERIOD_M,PI_2_Q24),POWERLPF_CF_M)

//speed regulator parameters
#define SPEED_REGULATOR_FREQ_M    SPEED_LOOP_CAL_FREQ   //speed regulator execution frequency (unit-Hz)
#define SPEED_REGULATOR_PERIOD_M  (_IQ(1.0)/SPEED_REGULATOR_FREQ_M)//speed regulator execution period (unit-s)
#define SPEED_REGULATOR_KP_M      _IQ(0.00128)          //speed regulator proportional coefficient (actual value)
#define SPEED_REGULATOR_KI_M      _IQ(0.001)            //speed regulator integral coefficient (actual value)
#define SPEED_REGULATOR_KD_M      _IQ(0.0)              //speed regulator derivative coefficient 0.0000001
#define SPEED_REGULATOR_KPADJ_M   _IQ(1.0)              //speed regulator proportional coefficient adjustment factor
#define SPEED_REGULATOR_KIADJ_M   _IQ(1.0)              //speed regulator integral coefficient adjustment factor
#define SPEED_REGULATOR_KDADJ_M   _IQ(1.0)              //speed regulator derivative coefficient adjustment factor
#define SPEED_REGULATOR_ULIM_M    _IQ(1.00)             //speed regulator output upper limit (unit-A_per)
#define SPEED_REGULATOR_LLIM_M    _IQ(0.00)             //speed regulator output lower limit (unit-A_per)
#define SPEED_REGULATOR_DLIM_M    _IQ(0.2)              //speed regulator single output limit (unit-A_per)

//current regulator parameters
//voltage limit definition
#ifdef MAG100_TABLE_USED
#define MAX_MODULATIONSQUARE        _IQ(1.0000)     //maximum modulation ratio 1.0 (unit-1)
#define MAX_MODULATION              _IQ(1.0000)     //maximum modulation ratio 1.0 (unit-1)
#elif  defined MAG098_TABLE_USED
#define MAX_MODULATIONSQUARE        _IQ(0.9604)     //maximum modulation ratio 0.98 (unit-1)
#define MAX_MODULATION              _IQ(0.9800)     //maximum modulation ratio 0.98 (unit-1)
#elif  defined MAG096_TABLE_USED
#define MAX_MODULATIONSQUARE        _IQ(0.9216)     //maximum modulation ratio 0.96 (unit-1)
#define MAX_MODULATION              _IQ(0.9600)     //maximum modulation ratio 0.96 (unit-1)
#elif  defined MAG094_TABLE_USED
#define MAX_MODULATIONSQUARE        _IQ(0.8836)     //maximum modulation ratio 0.94 (unit-1)
#define MAX_MODULATION              _IQ(0.9400)     //maximum modulation ratio 0.94 (unit-1)
#elif  defined MAG092_TABLE_USED
#define MAX_MODULATIONSQUARE        _IQ(0.8464)     //maximum modulation ratio 0.92 (unit-1)
#define MAX_MODULATION              _IQ(0.9200)     //maximum modulation ratio 0.92 (unit-1)
#elif  defined MAG090_TABLE_USED
#define MAX_MODULATIONSQUARE        _IQ(0.8100)     //maximum modulation ratio 0.90 (unit-1)
#define MAX_MODULATION              _IQ(0.9000)     //maximum modulation ratio 0.90 (unit-1)
#elif  defined MAG088_TABLE_USED
#define MAX_MODULATIONSQUARE        _IQ(0.7744)     //maximum modulation ratio 0.88 (unit-1)
#define MAX_MODULATION              _IQ(0.8800)     //maximum modulation ratio 0.88 (unit-1)
#elif  defined MAG086_TABLE_USED
#define MAX_MODULATIONSQUARE        _IQ(0.7396)     //maximum modulation ratio 0.86 (unit-1)
#define MAX_MODULATION              _IQ(0.8600)     //maximum modulation ratio 0.86 (unit-1)
#endif

#define MOTOR_VOLT_CAP_PU       _IQ(1.0)       //motor voltage cap (per-unit, 1.0=rated; at high bus voltage the limit radius=min(this value*bus correction, modulation upper limit), adjust according to motor withstand voltage)

#define CURR_REGULATOR_FREQ_M   FREQ_SVPWM                     //current regulator execution frequency (unit-Hz)
#define CURR_REGULATOR_BW_M     (uint16_t)((200*PI_2_Q16)>>16) //current regulator bandwidth (unit-rad/s)

#define DAXIS_CURR_REGULATOR_KD_M       _IQ(0.0)        //D-axis current regulator derivative coefficient
#define DAXIS_CURR_REGULATOR_KPADJ_M    _IQ(1.0)        //D-axis current regulator proportional coefficient adjustment factor
#define DAXIS_CURR_REGULATOR_KIADJ_M    _IQ(1.0)        //D-axis current regulator integral coefficient adjustment factor
#define DAXIS_CURR_REGULATOR_KDADJ_M    _IQ(1.0)        //D-axis current regulator derivative coefficient adjustment factor
#define DAXIS_CURR_REGULATOR_ULIM_M     _IQ(1.0)        //D-axis current regulator output upper limit (unit-V_per)
#define DAXIS_CURR_REGULATOR_LLIM_M     _IQ(-1.0)       //D-axis current regulator output lower limit (unit-V_per)
#define DAXIS_CURR_REGULATOR_DLIM_M     _IQ(1.0)        //current regulator single output limit (unit-V_per)

#define QAXIS_CURR_REGULATOR_KD_M       _IQ(0.0)        //Q-axis current regulator derivative coefficient
#define QAXIS_CURR_REGULATOR_KPADJ_M    _IQ(1.0)        //Q-axis current regulator proportional coefficient adjustment factor
#define QAXIS_CURR_REGULATOR_KIADJ_M    _IQ(1.0)        //Q-axis current regulator integral coefficient adjustment factor
#define QAXIS_CURR_REGULATOR_KDADJ_M    _IQ(1.0)        //Q-axis current regulator derivative coefficient adjustment factor
#define QAXIS_CURR_REGULATOR_ULIM_M     _IQ(1.0)        //Q-axis current regulator output upper limit (unit-V_per)
#define QAXIS_CURR_REGULATOR_LLIM_M     _IQ(-1.0)       //Q-axis current regulator output lower limit (unit-V_per)
#define QAXIS_CURR_REGULATOR_DLIM_M     _IQ(1.0)        //current regulator single output limit (unit-V_per)


//D-axis current control related parameters
#define DAXIS_CURR_REF_INIT1_M          _IQ(0.00)       //D-axis current reference initial value 1 (unit-A_per)
#define DAXIS_CURR_REF_INIT2_M          _IQ(0.00)       //D-axis current reference initial value 2 (unit-A_per)
#define DAXIS_CURR_REF_STEP1_M          _IQ(0.0001)     //D-axis current reference initial value adjustment step 1 (unit-A_per/s)
#define DAXIS_CURR_REF_STEP2_M          _IQ(0.0001)     //D-axis current reference initial value adjustment step 2 (unit-A_per/s)
#define DAXIS_CURR_WEAK_SPEED_M         _IQ(1.5)        //D-axis current overspeed field-weakening speed threshold for sudden load reduction (unit-rad/s_per)

#define QAXIS_CURR_REF_INIT1_M          _IQ(0.35)        //Q-axis current reference initial value 1 (unit-A_per)
/* Exported variables -------------------------------------------------------*/
extern Motor_PID_Type  Mpid_M;              //motor overall PID regulator structure
extern Motor_Power_Type Power_Cal;          //power loop structure
extern const _iq24 Magnitude_table[256];    //voltage limit table


/* Exported macro -----------------------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
//speed regulator and current regulator parameter definition
void PID_Prameter_Define (Motor_PID_Type * Mpid);

//speed regulator and current regulator output state initialization.
void PID_Output_Init(Motor_PID_Type * Mpid, int8_t Direction);

//speed regulator and current regulator parameter calculation
void PID_ParaBase_Cal_M(Motor_Base_Type *Basestruc,Motor_PID_Type * Mpid);

//d-axis current processing related parameter definition
void DCurrProcess_Parameter_Define(DCurrProcess_Type * DCurrProcess);

void PowerCal_Prameter_Define(Motor_Power_Type *MPower );

void PowerCal_Init(Motor_Power_Type *MPower);

#endif
