/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __M1_MOTOR_PARAMETER_H
#define __M1_MOTOR_PARAMETER_H

/* Includes -----------------------------------------------------------------*/
/* Exported types -----------------------------------------------------------*/
/* Exported constants -------------------------------------------------------*/

//Xiaomi fan
#define RATED_FREQ_M        100              //motor rated frequency (unit-Hz)
#define RATED_VOLT_M        _IQ16(8.4865629) //motor rated voltage (line voltage RMS, unit-V)
#define RATED_CURR_M        _IQ16(1.25)      //motor rated current (line current RMS, unit-A)
#define ROTOR_FLUX_M        _IQ(0.007643)    //motor permanent-magnet flux linkage (unit-Wb)
#define POLEPARE_M          5                //motor pole pairs (unit-1)
#define STATOR_RES_M        _IQ(0.98)        //motor stator resistance (unit-ohm)
#define STATOR_LD_M         _IQ(0.0020299)   //motor rotor d-axis inductance (unit-H)
#define STATOR_LQ_M         _IQ(0.0025204)   //motor rotor q-axis inductance (unit-H)

//motor stator resistance + sampling resistor (unit-ohm)
#define STATOR_RES_TOTAL_M  (STATOR_RES_M + (CURR_SAMPLE_RESISTOR_ADC<<8)+_IQ(0.08))

//dq-axis current low-pass filter parameters, used for windmill
#define IDQLPF_CF_HIGH_M           400                  //observed dq-axis current low-pass filter cutoff frequency_windmill (unit-Hz)400
#define IDQLPF_PERIOD_M            SMO_CAL_PERIOD_M    //observed dq-axis current low-pass filter calculation period_windmill (unit-s)
#define IDQLPF_WcT_HIGH_M          _IQmpyI32(_IQmpy(IDQLPF_PERIOD_M,PI_2_Q24),IDQLPF_CF_HIGH_M)

#define IDQLPF_HIGH_INPUT_COEF      _IQdiv(IDQLPF_WcT_HIGH_M,(IDQLPF_WcT_HIGH_M + UNIT_Q24))
#define IDQLPF_HIGH_OUTPUT_COEF     _IQdiv(UNIT_Q24,(IDQLPF_WcT_HIGH_M + UNIT_Q24));


//dq-axis current low-pass filter parameters, used for observer
#define IDQLPF_CF_OBS_M           400                  //observed dq-axis current low-pass filter cutoff frequency_observer (unit-Hz)200
#define IDQLPF_OBS_PERIOD_M       SMO_CAL_PERIOD_M    //observed dq-axis current low-pass filter calculation period_observer (unit-s)
#define IDQLPF_WcT_OBS_M          _IQmpyI32(_IQmpy(IDQLPF_OBS_PERIOD_M,PI_2_Q24),IDQLPF_CF_OBS_M)

#define IDQLPF_OBS_INPUT_COEF     _IQdiv(IDQLPF_WcT_OBS_M,(IDQLPF_WcT_OBS_M + UNIT_Q24))
#define IDQLPF_OBS_OUTPUT_COEF    _IQdiv(UNIT_Q24,(IDQLPF_WcT_OBS_M + UNIT_Q24));

/* Exported variables -------------------------------------------------------*/
extern Motor_Sructure_Type MStruc_M;            //motor overall control structure
extern Motor_Base_Type  MBase_M;                //motor base value parameter structure
extern  Runningstatus_Type  RunningStatus_M;    //system running state
extern  uint16_t volatile System_Status_Global;  //system status variable definition

/* Exported macro -----------------------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
//motor parameter definition
void Motor_Prameter_Define(Motor_Base_Type *Basestruc);

//motor status initialization
void Motor_Status_Initial_M(Motor_Sructure_Type *MStruc);

//motor global variable initialization
void Global_Status_Initial(void);

//motor phase filter parameter initialization
void Motor_FilerPara_Cal_M(Motor_Sructure_Type *MStruc);

//motor base value parameter calculation
void Motor_BaseCal_M(Motor_Base_Type *Basestruc);

#endif
