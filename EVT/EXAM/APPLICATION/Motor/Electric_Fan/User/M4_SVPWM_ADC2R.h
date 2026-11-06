/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __MOTOR_SVPWM_ADC1R_H
#define __MOTOR_SVPWM_ADC1R_H

/* Includes -----------------------------------------------------------------*/
/* Exported types -----------------------------------------------------------*/
/* Exported constants -------------------------------------------------------*/
//SVPWM parameters
#define DCBUS_RATED_VOLT    _IQ16(12)      //rated bus voltage (unit-V)

#define SYSTEM_CLOCK_FREQ   48000000uL     //system clock frequency (unit-Hz)

//SVPWM timer prescaler
#define TIMER_PRESCALER_SVPWM       0
//SVPWM timer clock frequency (unit-Hz)
#define TIMER_CLOCK_FREQ_SVPWM      (SYSTEM_CLOCK_FREQ/(TIMER_PRESCALER_SVPWM+1))
//SVPWM timer time update rate
#define TIMER_UPDATE_RATE_SVPWM     1
//SVPMW switching frequency (unit-Hz)
#define FREQ_SVPWM                  12000  //SVPMW switching frequency (unit-Hz) 

//SVPWM dead time (unit-ns)
#define DEADTIME_SVPWM   500               //SVPWM dead time (unit-ns)

//dead time register configuration value
#define DEADTIME_CONFIGURE  (uint16_t)(24)

#define BRK_GPIO_M    GPIOA            //emergency stop port group
#define BRK_PIN_M     GPIO_Pin_7       //emergency stop port

//ADC parameters
#define CURR_SAMPLE_RESISTOR_ADC    _IQ16(0.1)          	//current sampling resistor
#define CURR_AMPLIFIER_ADC          _IQ16(4.0)       		//current sampling hardware amplification factor

//current sampling range (amplitude, unit-A/per)
#define CURR_SAMPLE_RANGE_ADC       _IQ16div(_IQ16(2.5),_IQ16mpy(CURR_SAMPLE_RESISTOR_ADC,CURR_AMPLIFIER_ADC))

//ADC conversion clock configuration, sampling time 3.5 ADC clocks    146ns
#define CURR_SAMPLING_TIMER_CLOCK        ADC_SampleTime_CyclesMode0

#define CURR_OFFSET_READ_NUM_ADC    ((uint8_t)16)   //current sampling offset calculation count

//current sampling channel configuration
#define CURR_CHANNEL_PHASEA_M         ADC_Channel_OPA

//voltage sampling channel configuration
#define DCVOLT_CHANNEL_PHASEA_M         ADC_Channel_3
//voltage sampling configuration
/* bus voltage sampling resistor divider ratio */
#define DCVOLT_DIV_R_LOW_K          1.0f                             /* divider network pull-down resistor (kohm) */
#define DCVOLT_DIV_R_HIGH_K         4.7f                            /* divider network series resistor (kohm) */
#define DCVOLT_CONV_RATIO           _IQ16(DCVOLT_DIV_R_LOW_K / (DCVOLT_DIV_R_LOW_K + DCVOLT_DIV_R_HIGH_K))

#define DCVOLT_INIT                 _IQ(1.0)                        /*bus voltage sampling average initialization*/
#define DCVOLT_AVER_SIZE            32                              /*bus voltage average calculation count*/

/* Exported variables -------------------------------------------------------*/
extern SVPWM_2R_Type    SVPWM_Cal_M;    //SVPWN calculation structure
extern ADCStruc_Type    ADC_M;          //ADC sampling structure

/* Exported macro -----------------------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
//SVPWM parameter initialization
void SVPWM_2R_Paramter_Init(SVPWM_2R_Type *SVPWM_Cal,Motor_Base_Type *Mbase);

//SVPWM status initialization
void SVPWM_2R_Status_Init_M(SVPWM_2R_Type *SVPWM_Cal);

//ADC parameter calculation
void ADC_Parameter_Init (ADCStruc_Type *ADCStruc,Motor_Base_Type *Mbase);

//ADC status initialization
void ADC_Status_Init (ADCStruc_Type *ADCStruc);

//SVPWM timer configuration;
void SVPWM_2R_Timer_Configure(void);

//ADC and op-amp configuration
void ADC_OPA_Configure(ADCStruc_Type *ADCStruc);

//AD current sampling zero offset reading
void ADC_Offset_Reading(ADC_TypeDef *ADCx, ADCStruc_Type *ADCStruc,u_int8_t ADC_num);

//ADC conversion start
void ADC_Start (FunctionalState cmd);

//timer counter compare value update
void SVPWM_Update (SVPWM_2R_Type *SVPWM_Cal);

//bus voltage calculation
void DCBUS_Volt_Cal(ADCStruc_Type *ADCStruc,ADC_TypeDef *ADCx);

//SVPWM calculation
void SVPWM_Calculation(SVPWM_2R_Type *SVPWM_Cal, Component_2S_Type *VoltRef);

//two-resistor sampling current reading
Component_3S_Type Current_Cal_2R(ADCStruc_Type *ADCStruc);

//zero voltage output, effective immediately
void SVPWM_Zerovolt_Output(SVPWM_2R_Type *SVPWM_Cal);

//all low-side switches on, effective immediately
void SVPWM_Lowside_Effective(SVPWM_2R_Type *SVPWM_Cal);

//SVPWM all off
void SVPWM_OFF(void);

//SVPWM all on
void SVPWM_ON(void);

#endif
