/********************************** (C) COPYRIGHT  *******************************
* File Name          : Soft_IIC.h
* Author             : WCH
* Version            : V1.0.0
* Date               : 2025/08/01
* Description        : This file provides some prototypes and defines .
*********************************************************************************
* Copyright (c) 2025 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#ifndef __Soft_IIC_H
#define __Soft_IIC_H

#include "debug.h"

#define GPIO_Speed_High GPIO_Speed_30MHz

#define ACK_OK  1
#define ACK_ERR 0

void    Soft_IIC_GPIO_Config(void);
void    Soft_IIC_Delay(uint32_t);
void    Soft_IIC_Start(void);
void    Soft_IIC_Stop(void);
void    Soft_IIC_ACK(void);
void    Soft_IIC_NoACK(void);
uint8_t Soft_IIC_WaitACK(void);
uint8_t Soft_IIC_WriteByte(uint8_t IIC_Data);
uint8_t Soft_IIC_ReadByte(uint8_t ACK_Mode);
uint8_t Soft_IIC_ScanAddress(uint8_t *addr_list, uint8_t max_num);

#endif
