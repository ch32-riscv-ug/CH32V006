/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __M0_CONTROL_CONFIGURE_H
#define __M0_CONTROL_CONFIGURE_H
/* Includes -----------------------------------------------------------------*/
/* Exported types -----------------------------------------------------------*/
/* Exported constants -------------------------------------------------------*/
//Positioning sector selection
#define SIX_SECTOR_POSITION     //6-sector positioning

//Pulse positioning startup
// #define POSITION_EN

#define SPEED_LOOP 0
#define POWER_LOOP 1
#define CLOSE_LOOP  SPEED_LOOP

//
#define M007E8R6_FAN_1_1
// #define M007E8U6_FAN_1_1

//Voltage limit table selection, 8 configurations in total: 100, 098, 096, 094, 092, 090, 088, 086
//#define MAG100_TABLE_USED
//#define MAG098_TABLE_USED
//#define MAG096_TABLE_USED
#define MAG094_TABLE_USED
//#define MAG092_TABLE_USED
//#define MAG090_TABLE_USED
//#define MAG088_TABLE_USED
//#define MAG086_TABLE_USED

//Function selection conflict check
#if ( (!defined(MAG100_TABLE_USED))&&(!defined(MAG098_TABLE_USED))&&\
        (!defined(MAG096_TABLE_USED))&&(!defined(MAG094_TABLE_USED))\
        &&(!defined(MAG092_TABLE_USED))&&(!defined(MAG090_TABLE_USED))\
        &&(!defined(MAG088_TABLE_USED))&&(!defined(MAG086_TABLE_USED)))
#error "Invalid configuration: No voltage limit selected"
#endif


#if ( (defined(SIX_SECTOR_POSITION))&&(defined(TWELVE_SECTOR_POSITION)))
#error "Invalid configuration: Duplicate HFVQI configure"
#endif


/* Exported variables -------------------------------------------------------*/
/* Exported macro -----------------------------------------------------------*/
/* Exported functions -------------------------------------------------------*/

#endif
