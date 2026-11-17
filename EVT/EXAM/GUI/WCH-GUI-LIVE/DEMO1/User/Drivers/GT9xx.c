/********************************** (C) COPYRIGHT  *******************************
* File Name          : GT9xx.c
* Author             : WCH
* Version            : V1.0.0
* Date               : 2025/08/01
* Description        : This file provides some functions.
*********************************************************************************
* Copyright (c) 2025 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/

#include "GT9xx.h"

volatile TouchStructure touchInfo;

TouchChipInfo touchChipInfo;

#define GPIO_PIN_SET   1
#define GPIO_PIN_RESET 0

#define SUCCESS        0
#define ERROR          1

#define Touch_INT_PORT GPIOB
#define Touch_INT_PIN  GPIO_Pin_15

#define Touch_RST_PORT GPIOB
#define Touch_RST_PIN  GPIO_Pin_14

/* NOTE: Touch_RST/Touch_INT are PB14/PB15, which are LTDC_B0/LTDC_B1 on this
 * board. Do not call this function, otherwise the LCD blue channel would be
 * driven by the touch pins. The controller is polled over I2C only. */
/*********************************************************************
 * @fn      GT9XX_Reset
 *
 * @brief   Reset the GT9xx.
 *
 * @return  none
 */
void GT9XX_Reset(void)
{
    /* CH32V006K8U has no PB14/PB15; the touch controller is polled over
     * I2C only and its reset pin is driven by the board (if present). */
}

/*********************************************************************
 * @fn      GT9XX_WriteRegAddr
 *
 * @brief   Write the GT9xx register.
 *
 * @param   addr - the register address.
 *
 * @return  status - SUCCESS or ERROR.
 */
uint8_t GT9XX_WriteRegAddr(uint16_t addr)
{
    uint8_t status = ERROR;

    if (Soft_IIC_WriteByte(GT9XX_IIC_WADDR) == ACK_OK)
    {
        if (Soft_IIC_WriteByte((uint8_t)(addr >> 8)) == ACK_OK)
        {
            if (Soft_IIC_WriteByte((uint8_t)(addr)) == ACK_OK)
            {
                status = SUCCESS;
            }
        }
    }

    return status;
}

/*********************************************************************
 * @fn      GT9XX_WriteData
 *
 * @brief   Write the GT9xx data.
 *
 * @param   addr - the register address.
 *         Value - the data to be written.
 *
 * @return  status - SUCCESS or ERROR.
 */
uint8_t GT9XX_WriteData(uint16_t addr, uint8_t value)
{
    uint8_t status = ERROR;

    Soft_IIC_Start();

    if (GT9XX_WriteRegAddr(addr) == SUCCESS)
    {
        if (Soft_IIC_WriteByte(value) == ACK_OK)
        {
            status = SUCCESS;
        }
    }
    Soft_IIC_Stop();

    return status;
}

/*********************************************************************
 * @fn      GT9XX_WriteReg
 *
 * @brief   Write the GT9xx register.
 *
 * @param   addr - the register address.
 *         cnt  - the number of data.
 *         value - the data to be written.
 *
 * @return  status - SUCCESS or ERROR.
 */
uint8_t GT9XX_WriteReg(uint16_t addr, uint8_t cnt, uint8_t* value)
{
    uint8_t status = ERROR;
    uint8_t i;

    Soft_IIC_Start();

    if (GT9XX_WriteRegAddr(addr) == SUCCESS)
    {
        for (i = 0; i < cnt; i++)
        {
            Soft_IIC_WriteByte(value[i]);
        }

        status = SUCCESS;
    }
    Soft_IIC_Stop();
    return status;
}

/*********************************************************************
 * @fn      GT9XX_ReadReg
 *
 * @brief   Read the GT9xx register.
 *
 * @param   addr - the register address.
 *         cnt  - the number of data.
 *         value - the data to be written.
 *
 * @return  status - SUCCESS or ERROR.
 */
uint8_t GT9XX_ReadReg(uint16_t addr, uint8_t cnt, uint8_t* value)
{
    uint8_t status;
    uint8_t i;

    status = ERROR;
    Soft_IIC_Start();

    if (GT9XX_WriteRegAddr(addr) == SUCCESS)
    {
        Soft_IIC_Start();

        if (Soft_IIC_WriteByte(GT9XX_IIC_RADDR) == ACK_OK)
        {
            for (i = 0; i < cnt; i++)
            {
                if (i == (cnt - 1))
                {
                    value[i] = Soft_IIC_ReadByte(0);
                }
                else
                {
                    value[i] = Soft_IIC_ReadByte(1);
                }
            }
            status = SUCCESS;
        }
    }
    Soft_IIC_Stop();
    return status;
}

void IIC_ScanExample(void)
{
    uint8_t devices[16];
    uint8_t count, i;

    count = Soft_IIC_ScanAddress(devices, 16);

    if (count == 0)
    {
        printf("no iic devices\n");
    }
    else
    {
        printf("iic devices %d\n",count);
        for (i = 0; i < count; i++)
        {
            printf("get iic device  %x\n", devices[i] << 1);
        }
    }
}

/*********************************************************************
 * @fn      Touch_Init
 *
 * @brief   Initialize the GT9xx.
 *
 * @return  status - SUCCESS or ERROR.
 */
uint8_t Touch_Init(void)
{
    uint8_t GT9XX_Info[11];
    uint8_t cfgVersion = 0;

    Soft_IIC_GPIO_Config();


    IIC_ScanExample();

    
    // GT9XX_Reset();

    GT9XX_ReadReg(GT9XX_ID_ADDR, 11, GT9XX_Info);
    GT9XX_ReadReg(GT9XX_CFG_ADDR, 1, &cfgVersion);

    if (GT9XX_Info[0] == '9')
    {
        touchChipInfo.id[0] = (char)GT9XX_Info[0];
        touchChipInfo.id[1] = (char)GT9XX_Info[1];
        touchChipInfo.id[2] = (char)GT9XX_Info[2];
        touchChipInfo.id[3] = (char)GT9XX_Info[3];
        touchChipInfo.id[4] = '\0';
        touchChipInfo.fw_version = (uint16_t)(((uint16_t)GT9XX_Info[5] << 8) | GT9XX_Info[4]);
        touchChipInfo.width = (uint16_t)(((uint16_t)GT9XX_Info[7] << 8) | GT9XX_Info[6]);
        touchChipInfo.height = (uint16_t)(((uint16_t)GT9XX_Info[9] << 8) | GT9XX_Info[8]);
        touchChipInfo.ready = 1u;

        printf("Touch ID: GT%.4s \r\n", GT9XX_Info);
        printf("fireware version 0x%4x\r\n", (GT9XX_Info[5] << 8) + GT9XX_Info[4]);
        printf("Touch resolution :%d * %d\r\n", (GT9XX_Info[7] << 8) + GT9XX_Info[6], (GT9XX_Info[9] << 8) + GT9XX_Info[8]);
        printf("Touch parameter version  0x%2x \r\n", cfgVersion);
        return SUCCESS;
    }
    else
    {
        printf("Touch Error\r\n");

        return ERROR;
    }
}

/*********************************************************************
 * @fn      Touch_Init
 *
 * @brief   Initialize the GT9xx.
 *
 * @return  None.
 */
void Touch_Scan(void)
{
    uint8_t status = 0;
    uint8_t touchData[1 + 8 * TOUCH_MAX];
    uint8_t num;
    uint8_t i;

    if (GT9XX_ReadReg(GT9XX_READ_ADDR, 1, &status) != SUCCESS)
    {
        touchInfo.flag = 0;
        touchInfo.num = 0;
        return;
    }

    if ((status & 0x80u) == 0u)
    {
        return;
    }

    num = (uint8_t)(status & 0x0fu);
    if (num > TOUCH_MAX)
    {
        num = TOUCH_MAX;
    }

    if (num == 0u)
    {
        touchInfo.flag = 0;
        touchInfo.num = 0;
        GT9XX_WriteData(GT9XX_READ_ADDR, 0);
        return;
    }

    GT9XX_ReadReg(GT9XX_READ_ADDR, (uint8_t)(1u + 8u * num), touchData);
    GT9XX_WriteData(GT9XX_READ_ADDR, 0);

    touchInfo.num = num;

    for (i = 0; i < num; i++)
    {
        touchInfo.y[i] = (touchData[5 + 8 * i] << 8) | touchData[4 + 8 * i];
        touchInfo.x[i] = (touchData[3 + 8 * i] << 8) | touchData[2 + 8 * i];
    }
    touchInfo.flag = 1;
}
