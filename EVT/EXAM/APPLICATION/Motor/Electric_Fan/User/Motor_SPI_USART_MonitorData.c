/* Includes -----------------------------------------------------------------*/
#include "Motor_SPI_USART_MonitorData.h"
/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private functions --------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Variables ----------------------------------------------------------------*/
t_sendUsbData sendUsbData = {0,0,0,0,0,0,0};
const u_int8_t usbFrameByteNum[12] = {0, 32, 32, 28, 28, 24, 28, 32, 20, 22, 24, 26};
const u_int8_t needSendLen[12] = {0, USB_NEEDSEND_LEN1, USB_NEEDSEND_LEN2, USB_NEEDSEND_LEN3, USB_NEEDSEND_LEN4, USB_NEEDSEND_LEN5,
        USB_NEEDSEND_LEN6, USB_NEEDSEND_LEN7, USB_NEEDSEND_LEN8, USB_NEEDSEND_LEN9, USB_NEEDSEND_LEN10, USB_NEEDSEND_LEN11};

//__attribute__((section(".highcode")))
void SPI_Deal_sendMonitorData(void)
{
    u_int16_t data=0;
    u_int8_t i=0, frameByteNum=0;
    uint8_t reDataLen;
    if(sendUsbData.head >= sendUsbData.end)
    {
        reDataLen = sendUsbData.head - sendUsbData.end;
    }
    else
    {
        reDataLen = (uint16_t)USB_SENDBUF_LEN - sendUsbData.end + sendUsbData.head;
    }
    sendUsbData.sendDataLen=5;
    if(sendUsbData.sendDataLen)
    {
        if(reDataLen >= needSendLen[sendUsbData.sendDataLen])
        {
            GPIOC->BCR = GPIO_Pin_7;                                      
            __asm volatile ("nop");                                       
            while((SPI1->STATR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET);   

            SPI1->DATAR = 0x55AA;                                         
            while((SPI1->STATR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET);   

            sendUsbData.heartBeat++;
            data=sendUsbData.heartBeat<<8;
            data|=usbFrameByteNum[sendUsbData.sendDataLen];
            SPI1->DATAR = data;                                            
            while((SPI1->STATR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET);    

            frameByteNum = 4;
            while(frameByteNum < usbFrameByteNum[sendUsbData.sendDataLen])
            {
                for(i = 0; i < sendUsbData.sendDataLen; i++)
                {
                    SPI1->DATAR=sendUsbData.sendDataBuf[sendUsbData.end+i];
                    while((SPI1->STATR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET);    
                    frameByteNum+=2;
                }
                sendUsbData.end += MaxUsbDataLen;
            }

            while( (SPI1->STATR & SPI_I2S_FLAG_BSY) != (uint16_t)RESET);    
            __asm volatile ("nop");                                         
            GPIOC->BSHR = GPIO_Pin_7;                                     
        }
    }
}
void SPI_FullDuplex_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure={0};
    SPI_InitTypeDef SPI_InitStructure={0};

    RCC_PB2PeriphClockCmd( RCC_PB2Periph_GPIOC | RCC_PB2Periph_SPI1|RCC_PB2Periph_AFIO, ENABLE );

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init( GPIOC, &GPIO_InitStructure );

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_SetBits(GPIOC,GPIO_Pin_7);   

    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;    
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_16b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;//48/4=12M
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_LSB;
    SPI_Init( SPI1, &SPI_InitStructure );

    GPIO_ResetBits(GPIOC,GPIO_Pin_7); 
    SPI_Cmd( SPI1, ENABLE );          
}


#define USATR1_PD5 0
#define USATR1_PC0 1

#if   defined (M007E8R6)
    #define USATR_IO    USATR1_PD5
#elif defined (M007E8U6)
    #define USATR_IO    USATR1_PC0
#endif

void Usart_Init(uint32_t Baud)
{
    GPIO_InitTypeDef GPIO_InitStructure={0};
    USART_InitTypeDef USART_InitStructure = {0};

    RCC_PB2PeriphClockCmd(RCC_PB2Periph_GPIOC|RCC_PB2Periph_GPIOD | RCC_PB2Periph_USART1 | RCC_PB2Periph_AFIO, ENABLE);

#if (USATR_IO==USATR1_PC0)
    GPIO_PinRemapConfig(GPIO_PartialRemap3_USART1, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
#elif (USATR_IO == USATR1_PD5)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
#endif

    USART_InitStructure.USART_BaudRate = Baud;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx ;

    USART_Init(USART1, &USART_InitStructure);
    USART_Cmd(USART1, ENABLE);

}
/******************************************************************************
* Function Name  : Usart_Deal_sendMonitorData
* Description    : Usart send
* Input          : None
* Output         : None
* Return         : None
******************************************************************************/
void Usart_Deal_sendMonitorData(void)
{
        uint8_t i=0, frameByteNum=0;
        uint8_t reDataLen;
        sendUsbData.sendDataLen=3;
        if(sendUsbData.sendDataLen)
        {
            if(sendUsbData.head >= sendUsbData.end)
            {
                reDataLen = sendUsbData.head - sendUsbData.end;
            }
            else
            {
                reDataLen = (uint16_t)USB_SENDBUF_LEN - sendUsbData.end + sendUsbData.head;
            }
            if(reDataLen >= needSendLen[sendUsbData.sendDataLen])
            {
            __asm volatile ("nop");                                         
            while((USART1->STATR & USART_FLAG_TXE) == (uint16_t)RESET);     
            USART1->DATAR=0xAA;
            while((USART1->STATR & USART_FLAG_TXE) == (uint16_t)RESET);      
            USART1->DATAR=0x55;
            while((USART1->STATR & USART_FLAG_TXE) == (uint16_t)RESET);      
            USART1->DATAR=usbFrameByteNum[sendUsbData.sendDataLen];
            while((USART1->STATR & USART_FLAG_TXE) == (uint16_t)RESET);      
            sendUsbData.heartBeat++;
            USART1->DATAR=sendUsbData.heartBeat;
            while((USART1->STATR & USART_FLAG_TXE) == (uint16_t)RESET);      
            frameByteNum = 4;
            while(frameByteNum < usbFrameByteNum[sendUsbData.sendDataLen])//{0, 32, 32, 28, 28, 24, 28, 32, 20, 22, 24, 26};
            {
                for(i = 0; i < sendUsbData.sendDataLen; i++)
                {
                    USART1->DATAR = (uint8_t)((sendUsbData.sendDataBuf[sendUsbData.end+i])&0x00FF);
                    frameByteNum++;
                    while((USART1->STATR & USART_FLAG_TXE) == (uint16_t)RESET);      
                    USART1->DATAR = (uint8_t)((sendUsbData.sendDataBuf[sendUsbData.end+i])>>8);
                    frameByteNum++;
                    while((USART1->STATR & USART_FLAG_TXE) == (uint16_t)RESET);      
                }
                sendUsbData.end += MaxUsbDataLen;
            }

            while((USART1->STATR & USART_FLAG_TXE) == (uint16_t)RESET);      
            __asm volatile ("nop");                                         
        }
    }
}