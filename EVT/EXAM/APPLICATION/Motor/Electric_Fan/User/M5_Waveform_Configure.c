/* Includes -----------------------------------------------------------------*/
#include "M0_Control_Library.h"

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private functions --------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Variables ----------------------------------------------------------------*/
//extern uint16_t K1,K2;

/******************************************************************************
* Function Name  : Waveform_Display
* Description    : waveform display variable setting
* Input          : None
* Return         : None
******************************************************************************/
//__attribute__((section(".highcode")))
void Waveform_Display (void)
{
    //observation
    sendUsbData.sendDataBuf[sendUsbData.head+0] = (uint16_t)(System_Status_Global);
    sendUsbData.sendDataBuf[sendUsbData.head+1] = (uint16_t)(SMO_M.Cal_Struc.Angspeed_Aver2>>10);
    sendUsbData.sendDataBuf[sendUsbData.head+2] = (uint16_t)(SMO_M.Cal_Struc.CalphaNew>>10);
    sendUsbData.sendDataBuf[sendUsbData.head+3] = (uint16_t)(SMO_M.Cal_Struc.EalphaNew>>10);
    sendUsbData.sendDataBuf[sendUsbData.head+4] = (uint16_t)(MStruc_M.C3S.PhaseA>>10);

    sendUsbData.head += MaxUsbDataLen;
}
