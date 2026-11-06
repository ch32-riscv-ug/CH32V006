/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __M6_SPEED_POWER_LOOP_H
#define __M6_SPEED_POWER_LOOP_H

/* Includes -----------------------------------------------------------------*/
/* Exported types -----------------------------------------------------------*/

/* Exported constants -------------------------------------------------------*/
//Timer clock frequency used by the speed loop (unit-Hz)
#define TIMER_CLOCK_FREQ_SPEED      48000000uL

//Interrupt frequency of the speed loop timer (unit-Hz)
#define TIMER_FREQ_SPEED_LOOP       2000

//SYS timer period value
#define TIMER_PERIDO_SYSTICK     TIMER_CLOCK_FREQ_SPEED / (u32)(TIMER_FREQ_SPEED_LOOP*8)

//Calculation frequency of the speed loop (unit-Hz)
#define SPEED_LOOP_CAL_FREQ         2000

//Calculation period of the speed loop (unit-s)
#define SPEED_LOOP_CAL_PERIOD      _IQ(1.0)/SPEED_LOOP_CAL_FREQ

//Calculation period of the speed loop PID regulator (x+1)*500usec
#define SPEED_SAMPLE_PID_PERIOD     TIMER_FREQ_SPEED_LOOP/SPEED_LOOP_CAL_FREQ

/* Exported variables -------------------------------------------------------*/

/* Exported macro -----------------------------------------------------------*/

/* Exported functions -------------------------------------------------------*/
//Speed processing timer initialization
void Systick_Init(void);

//Speed processing timer enable
void SYS_TIM_Cmd(FunctionalState NewState);

#endif
