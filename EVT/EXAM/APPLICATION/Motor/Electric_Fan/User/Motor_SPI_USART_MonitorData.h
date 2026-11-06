/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __MOTOR_SPI_USART_MONITORDATA_H_
#define __MOTOR_SPI_USART_MONITORDATA_H_
/* Includes -----------------------------------------------------------------*/
#include "debug.h"
/* Exported constants -------------------------------------------------------*/
#define MaxUsbDataLen                  8
#define USB_SENDBUF_LEN         (MaxUsbDataLen<<5)
#define USB_NEEDSEND_LEN1       (MaxUsbDataLen*14)
#define USB_NEEDSEND_LEN2       (MaxUsbDataLen*7)
#define USB_NEEDSEND_LEN3       (MaxUsbDataLen*4)
#define USB_NEEDSEND_LEN4       (MaxUsbDataLen*3)
#define USB_NEEDSEND_LEN5       (MaxUsbDataLen*2)
#define USB_NEEDSEND_LEN6       (MaxUsbDataLen*2)
#define USB_NEEDSEND_LEN7       (MaxUsbDataLen*2)
#define USB_NEEDSEND_LEN8       (MaxUsbDataLen)
#define USB_NEEDSEND_LEN9       (MaxUsbDataLen)
#define USB_NEEDSEND_LEN10      (MaxUsbDataLen)
#define USB_NEEDSEND_LEN11      (MaxUsbDataLen)

/* Exported types -----------------------------------------------------------*/
typedef struct
{
    u_int8_t heartBeat;                        //---Heartbeat packet---
    u_int8_t delayNum;                         //---Delayed transmission---
    u_int8_t volatile sendDataLen;             //---Length of the data to be sent---
    u_int8_t curWriteFrame;                    //---Current write data frame---
    u_int8_t recDataLen;                       //---Length of the data received from host---
    u_int8_t end;                              //---Index of the sent position in send buffer---
    u_int8_t volatile head;                    //---Index of the write position in send buffer---
    u_int8_t recDataBuf[16];                   //---Receive data buffer---
    u_int8_t sendDataType[MaxUsbDataLen];      //---Send data type---
    u_int16_t sendDataBuf[USB_SENDBUF_LEN];    //---Send data buffer---
} t_sendUsbData;

/* Exported variables -------------------------------------------------------*/
extern t_sendUsbData sendUsbData;

/* Exported macro -----------------------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
extern void Deal_sendMonitorData(void);
extern void SPI_FullDuplex_Init(void);
extern void SPI_Deal_sendMonitorData(void);
extern void Usart_Init(uint32_t Baud);
extern void Usart_Deal_sendMonitorData(void);
#endif
