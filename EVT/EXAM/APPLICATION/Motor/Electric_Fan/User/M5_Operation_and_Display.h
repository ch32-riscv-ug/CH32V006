/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __MOTOR_OPERATION_AND_DISPLAY_H
#define __MOTOR_OPERATION_AND_DISPLAY_H

/* Includes -----------------------------------------------------------------*/
/* Exported types -----------------------------------------------------------*/
typedef enum {PRESSUP = 0, PRESSDOWN = 1} PressStatus;  //key pressed status
typedef struct
{
    GPIO_TypeDef *Group;         //IO group
    uint16_t Pin;                //IO pin
    uint16_t Effectivetime;      //effective time threshold
    uint16_t Effectivecnt;       //effective time count
    uint16_t Ineffectivetime;    //ineffective time threshold
    uint16_t Ineffectivecnt;     //ineffective time count
    Truth_Verify_Type Status;    //valid status
    PressStatus KeyPress;        //pressed status
} Keyrecover_Type;  //recoverable key structure

typedef struct
{
    Keyrecover_Type RUN;            //run key
    Keyrecover_Type Speedadjust;    //speed adjustment key
} Key_Manager_Type;  //recoverable key manager structure

//operation structure prototype definition
typedef struct
{
    //stop wait related variable definition
    uint16_t StopwaitCnt;           //stop wait counter variable
    uint16_t StopwaitSet;           //stop wait time setting

    //key time variable definition
    uint16_t StartbuttonSet;        //start key time setting
    uint16_t SpeedbuttonCnt;        //speed adjustment key counter variable
    uint16_t SpeedbuttonSetdown;    //speed adjustment key time setting lower limit

    //fault operation related variables
    uint16_t FaultHoldingSet;       //fault hold judgement count setting value
    uint16_t FaultHoldingCnt;       //fault hold counter value
    Truth_Verify_Type Startmark;    //startup flag
    uint8_t Checkmode;              //key function state, 0-start/stop, 1-speed adjustment
} Operation_Type;

//ramp control structure
typedef struct
{
    _iq24 Preramp;                    //pre-ramp reference (unit-rad/s_per)/(unit-W/s_per)
    _iq24 Aftramp;                    //post-ramp reference (unit-rad/s_per)/(unit-W/s_per)
    _iq24 AccStep;                    //acceleration step (unit-rad/s^2_per)/(unit-W/s^2_per)
    _iq24 DecStep;                    //deceleration step (unit-rad/s^2_per)/(unit-W/s^2_per)

    uint8_t Point_adncnt;             //adjustment counter value
    int8_t Direction;                 //speed direction

    Truth_Verify_Type Startfinish;    //startup complete flag
    Truth_Verify_Type Decemark;       //deceleration stop flag
} Rampctr_Type;
/* Exported constants -------------------------------------------------------*/
// operation definition


#define STARTUP_DIRECTION       1                       //running direction (1: forward, -1: reverse)
#define MAX_ANGSPEED_POS_M      _IQ(1.0)                //allowed maximum forward running speed (unit-per)
#define MAX_ANGSPEED_NEG_M      _IQ(-1.0)               //allowed maximum reverse running speed (unit-per)
#define MIN_ANGSPEED_POS_M      _IQ(0.025)              //allowed minimum forward running speed (unit-per)
#define MIN_ANGSPEED_NEG_M      _IQ(-0.025)             //allowed minimum reverse running speed (unit-per)
#define SPEED_RAMP_FREQ_M       SPEED_LOOP_CAL_FREQ     //acceleration/deceleration processing frequency (unit-Hz)
#define ACCE_TIME_M             5.0                     //acceleration time (unit-s)
#define DECE_TIME_M             5.0                     //deceleration time (unit-s)

#define SPEEDPOINT1             _IQ(0.3)                //speed 1 (unit-per)
#define SPEEDPOINT2             _IQ(0.5)               //speed 2 (unit-per)
#define SPEEDPOINT3             _IQ(0.8)               //speed 3 (unit-per)


#define POWERPOINT1             _IQ(0.3)               //power 1 (unit-per)
#define POWERPOINT2             _IQ(0.5)               //power 2 (unit-per)
#define POWERPOINT3             _IQ(0.8)               //power 3 (unit-per)

#define POINT_DEFAULT      1                            //default power-on gear
#define POINT_RANK         3                            //number of speed adjustment gears

//braking
#define FAST_DECE_SPEED         _IQ(0.25)               //fast deceleration speed target (unit-per)
#define BREAKSPEED              _IQ(0.10)               //braking speed (unit-per)

#define FAST_DECE_POWER         _IQ(0.15)               //fast deceleration power target (unit-per)
#define BREAKPOWER              _IQ(0.10)               //braking power speed (unit-per)

#define BREAKTIME               1.0                     //braking time (unit-s) 0.25

#define START_FINISH_SPEED      (FAST_DECE_SPEED +_IQ(0.05)) //startup complete judgement speed (unit-per)

//key parameters
#define KEY_CHECK_FREQ           SPEED_LOOP_CAL_FREQ    //key detection calculation frequency
#define SRKEY_EFFECTIVE_TIME     0.05                   //start/stop key time (unit-s)
#define SRKEY_INEFFECTIVE_TIME   0.05                   //speed adjustment key time lower limit (unit-s)

//LED parameters
#define LED_PROCESS_FREQ        SPEED_LOOP_CAL_FREQ     //gear detection calculation frequency
#define LED_FLICKER_TIME        0.5                     //LED flicker time interval (unit-s)

//operation parameters
#define OPERATION_CAL_FREQ      SPEED_LOOP_CAL_FREQ     //operation calculation frequency
#define STOP_WAITING_TIME       0.1                     //wait time after stop (unit-s)
#define FAULT_HOLDING_TIME      0.5                     //fault hold time (unit-s)

//power on/off control parameters
#define POWERVERY_FREQ              SPEED_LOOP_CAL_FREQ //judgement frequency (unit-Hz)
#define POWERON_VOLT                _IQ(0.75)            //power-on voltage threshold (unit-Vper)
#define POWERON_TIME                0.1                 //power-on judgement time (unit-s)
#define POWEROFF_VOLT               _IQ(0.65)            //power-off voltage threshold 2 (unit-Vper)
#define POWERVERY_DCVOLTAVERSIZE    4                   //bus voltage average calculation count for power on/off judgement

//buzzer control parameters
#define BUZZ_PROCESS_FREQ           SPEED_LOOP_CAL_FREQ //judgement frequency (unit-Hz)
#define BUZZ_EFFECTIVE_TIME1        0.3                 //effective time 1 (unit-s)
#define BUZZ_EFFECTIVE_TIME2        0.6                 //effective time 1 (unit-s)

#define BUZZ_TIMER_PRESCALER      1                     //buzzer timer prescaler
//buzzer timer clock frequency (unit-Hz)
#define BUZZ_TIMER_CLOCK_FREQ      (SYSTEM_CLOCK_FREQ/(BUZZ_TIMER_PRESCALER+1))
#define BUZZ_TIMER_FREQ           4000                  //buzzer switching frequency (unit-Hz)
//buzzer timer period
#define BUZZ_TIMER_PERIOD         BUZZ_TIMER_CLOCK_FREQ/BUZZ_TIMER_FREQ


/* Exported variables -------------------------------------------------------*/
extern Rampctr_Type         Ramp_M;        //motor acceleration/deceleration control structure
extern Operation_Type       Operation_M;        //system operation structure
extern Powercontrol_Type    Powerctr_M;         //power on/off control structure
extern Key_Manager_Type     Key_M;              //key manager structure
/* Exported macro -----------------------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
//motor operation processing
void Motor_Operation_Process(void);

//ramp control parameter definition
void rampctr_Parameter_Define(Rampctr_Type *Ramp_M);

//ramp status initialization
void rampctr_Initial(Rampctr_Type *Ramp_M);

// operation parameter definition
void Operation_Parameter_Define(Operation_Type *Operation);

//operation status initialization
void Operation_Status_Init(Operation_Type *Operation);

//ramp setting
void Ramp_Set(Rampctr_Type *Speed_Ramp);

//key detection
void KeyandLED_IOInit(void);
void KeyandLED_Parameter_Define(Key_Manager_Type *KeyM);
void KeyandLED_Status_Init(Key_Manager_Type *KeyM);
void Skeycheck_Recover(Keyrecover_Type *Keyrec);
void KeyCheck_Process(Key_Manager_Type *KeyM, Rampctr_Type *Speed_Ramp);

//power on/off parameter and status initialization
void Poweroperation_Initial(Powercontrol_Type *PowerOper);

//power on/off control logic
void Poweroperation_Process(Rampctr_Type *SpeedRamp,ADCStruc_Type *ADCStruc,Powercontrol_Type *PowerOper,\
                            Runningstatus_Type *Runningstatus,_iq24 Speedact);

void Break_Verify(Rampctr_Type *SpeedRamp, Runningstatus_Type *Runningstatus);

//speed ramp processing
void Speed_Ramp_Process(Rampctr_Type *Speed_Ramp);

//power ramp processing
void Power_Ramp_Process(Rampctr_Type *Power_Ramp);

//program for variable decrement processing based on interrupt time
void Time_Delay_Process(void);

//wait time setting function, timebase is the interrupt period of processing count
void Time_Wait(uint16_t);

//timing time setting function, timebase is the interrupt period of processing count
void Time_Delay_Set(uint16_t Delayset, uint16_t *Delaycounter);

//timing elapse judgement function, timebase is the interrupt period of processing count
Truth_Verify_Type Time_Delay_Elapse(uint16_t Delaycounter);

#endif
