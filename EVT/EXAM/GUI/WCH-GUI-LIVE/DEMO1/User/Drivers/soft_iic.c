/********************************** (C) COPYRIGHT  *******************************
* File Name          : Soft_IIC.c
* Author             : WCH
* Version            : V1.0.0
* Date               : 2025/08/01
* Description        : This file provides some functions.
*********************************************************************************
* Copyright (c) 2025 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/

#include "soft_iic.h"

#define GPIO_PIN_SET      1
#define GPIO_PIN_RESET    0

#define Soft_IIC_SCL_PORT GPIOB
#define Soft_IIC_SCL_PIN  GPIO_Pin_5

#define Soft_IIC_SDA_PORT GPIOB
#define Soft_IIC_SDA_PIN  GPIO_Pin_6

#define IIC_DelayVaule  (4) 

#define Soft_IIC_SCL(a)   GPIO_WriteBit(Soft_IIC_SCL_PORT, Soft_IIC_SCL_PIN, a)

#define Soft_IIC_SDA(a)   GPIO_WriteBit(Soft_IIC_SDA_PORT, Soft_IIC_SDA_PIN, a)


/*********************************************************************
 * @fn      Soft_IIC_GPIO_Config
 *
 * @brief   Configure the GPIO pins for Soft_IIC.
 *
 * @param   addr - the register address.
 *         Value - the data to be written.
 *
 * @return  None.
 */
void Soft_IIC_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_PB2PeriphClockCmd(RCC_PB2Periph_AFIO, ENABLE);
    RCC_PB2PeriphClockCmd(RCC_PB2Periph_GPIOB, ENABLE);
    RCC_PB2PeriphClockCmd(RCC_PB2Periph_GPIOC, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = Soft_IIC_SCL_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_High;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_OD;
    GPIO_Init(Soft_IIC_SCL_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin   = Soft_IIC_SDA_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_High;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_OD;
    GPIO_Init(Soft_IIC_SDA_PORT, &GPIO_InitStructure);

    GPIO_WriteBit(Soft_IIC_SCL_PORT, Soft_IIC_SCL_PIN, GPIO_PIN_SET);
    GPIO_WriteBit(Soft_IIC_SDA_PORT, Soft_IIC_SDA_PIN, GPIO_PIN_SET);
}

/*********************************************************************
 * @fn      Soft_IIC_Delay
 *
 * @brief   Delay function.
 *
 * @param   cnt - the delay time.
 *
 * @return  None.
 */
void Soft_IIC_Delay(uint32_t a)
{
    volatile uint16_t i;
    while (a--)
    {
        for (i = 0; i < 30; i++)
        {
            __NOP();
        }
    }
}

/*********************************************************************
 * @fn      Soft_IIC_Start
 *
 * @brief   Start condition.
 *
 * @return  None.
 */
void Soft_IIC_Start(void)
{
    Soft_IIC_SDA(1);
    Soft_IIC_SCL(1);
    Soft_IIC_Delay(IIC_DelayVaule);

    Soft_IIC_SDA(0);
    Soft_IIC_Delay(IIC_DelayVaule);
    Soft_IIC_SCL(0);
    Soft_IIC_Delay(IIC_DelayVaule);
}

/*********************************************************************
 * @fn      Soft_IIC_Stop
 *
 * @brief   Stop condition.
 *
 * @return  None.
 */
void Soft_IIC_Stop(void)
{
    Soft_IIC_SCL(0);
    Soft_IIC_Delay(IIC_DelayVaule);
    Soft_IIC_SDA(0);
    Soft_IIC_Delay(IIC_DelayVaule);

    Soft_IIC_SCL(1);
    Soft_IIC_Delay(IIC_DelayVaule);
    Soft_IIC_SDA(1);
    Soft_IIC_Delay(IIC_DelayVaule);
}

/*********************************************************************
 * @fn      Soft_IIC_ACK
 *
 * @brief   ACK condition.
 *
 * @return  None.
 */
void Soft_IIC_ACK(void)
{
    Soft_IIC_SCL(0);
    Soft_IIC_Delay(IIC_DelayVaule);
    Soft_IIC_SDA(0);
    Soft_IIC_Delay(IIC_DelayVaule);
    Soft_IIC_SCL(1);
    Soft_IIC_Delay(IIC_DelayVaule);

    Soft_IIC_SCL(0);
    Soft_IIC_SDA(1);

    Soft_IIC_Delay(IIC_DelayVaule);
}

/*********************************************************************
 * @fn      Soft_IIC_NoACK
 *
 * @brief   NoACK condition.
 *
 * @return  None.
 */
void Soft_IIC_NoACK(void)
{
    Soft_IIC_SCL(0);
    Soft_IIC_Delay(IIC_DelayVaule);
    Soft_IIC_SDA(1);
    Soft_IIC_Delay(IIC_DelayVaule);
    Soft_IIC_SCL(1);
    Soft_IIC_Delay(IIC_DelayVaule);

    Soft_IIC_SCL(0);
    Soft_IIC_Delay(IIC_DelayVaule);
}

/*********************************************************************
 * @fn      Soft_IIC_WaitACK
 *
 * @brief   Wait ACK condition.
 *
 * @return  None.
 */
uint8_t Soft_IIC_WaitACK(void)
{
    Soft_IIC_SDA(1);
    Soft_IIC_Delay(IIC_DelayVaule);
    Soft_IIC_SCL(1);
    Soft_IIC_Delay(IIC_DelayVaule);

    if (GPIO_ReadInputDataBit(Soft_IIC_SDA_PORT, Soft_IIC_SDA_PIN) != 0)
    {
        Soft_IIC_SCL(0);
        Soft_IIC_Delay(IIC_DelayVaule);
        return ACK_ERR;
    }
    else
    {
        Soft_IIC_SCL(0);
        Soft_IIC_Delay(IIC_DelayVaule);
        return ACK_OK;
    }
}

/*********************************************************************
 * @fn      Soft_IIC_WriteByte
 *
 * @brief   Write a byte.
 *
 * @param   IIC_Data - the data to be written.
 *
 * @return  None.
 */
uint8_t Soft_IIC_WriteByte(uint8_t IIC_Data)
{
    uint8_t i;

    for (i = 0; i < 8; i++)
    {
        Soft_IIC_SDA(IIC_Data & 0x80);

        Soft_IIC_Delay(IIC_DelayVaule);
        Soft_IIC_SCL(1);
        Soft_IIC_Delay(IIC_DelayVaule);
        Soft_IIC_SCL(0);
        if (i == 7)
        {
            Soft_IIC_SDA(1);
        }
        IIC_Data <<= 1;
    }

    return Soft_IIC_WaitACK();
}

/*********************************************************************
 * @fn      Soft_IIC_ReadByte
 *
 * @brief   Read a byte.
 *
 * @param   ACK_Mode - the ACK mode.
 *
 * @return  None.
 */
uint8_t Soft_IIC_ReadByte(uint8_t ACK_Mode)
{
    uint8_t IIC_Data = 0;
    uint8_t i        = 0;

    for (i = 0; i < 8; i++)
    {
        IIC_Data <<= 1;

        Soft_IIC_SCL(1);
        Soft_IIC_Delay(IIC_DelayVaule);
        IIC_Data |= (GPIO_ReadInputDataBit(Soft_IIC_SDA_PORT, Soft_IIC_SDA_PIN) & 0x01);
        Soft_IIC_SCL(0);
        Soft_IIC_Delay(IIC_DelayVaule);
    }

    if (ACK_Mode == 1)
        Soft_IIC_ACK();
    else
        Soft_IIC_NoACK();

    return IIC_Data;
}


/*********************************************************************
 * @fn      Soft_IIC_ScanAddress
 *
 * @brief   Scan I2C bus for available device addresses.
 *
 * @param   addr_list - array to store found addresses.
 *         max_num   - maximum number of addresses to store.
 *
 * @return  The number of devices found.
 */
uint8_t Soft_IIC_ScanAddress(uint8_t *addr_list, uint8_t max_num)
{
    uint8_t i;
    uint8_t device_count = 0;

    for (i = 0; i < 0x80; i++)   
    {
        Soft_IIC_Start();
        
        if (Soft_IIC_WriteByte(i << 1) == ACK_OK)
        {
            // 收到ACK，说明该地址有设备
            if (addr_list != NULL && device_count < max_num)
            {
                addr_list[device_count] = i;
            }
            device_count++;
        }
        
        Soft_IIC_Stop();
    }

    return device_count;
}