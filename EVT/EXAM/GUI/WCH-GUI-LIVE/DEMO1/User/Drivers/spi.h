/*********************************************************************
 * File Name          : spi.h
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2023/8/15
 * Description        : file for spi.
 *********************************************************************************
* Copyright (c) 2023 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#ifndef __SPI_H__
#define __SPI_H__

#include "debug.h"
#include "config.h"

/* CH32V006 has a single SPI peripheral; the LCD and the external SPI
 * flash share SPI1 and are selected by their own chip-select pins. */
#define SPI2                 SPI1

#define SPI1_DMA_RX_CH   DMA1_Channel2
#define SPI1_DMA_TX_CH   DMA1_Channel3

#define SPI1_DMA_TC_FLAG DMA1_FLAG_TC3
#define SPI1_DMA_RC_FLAG DMA1_FLAG_TC2

#define SPI2_DMA_RX_CH   DMA1_Channel2
#define SPI2_DMA_TX_CH   DMA1_Channel3

#define SPI2_DMA_TC_FLAG DMA1_FLAG_TC3
#define SPI2_DMA_RC_FLAG DMA1_FLAG_TC2

void SPI1_Init(void);
void SPI1_Read_DMA_Init(void);
void SPI1_Read_DMA(uint8_t *data, uint16_t len);

void SPI2_Init(void);
void SPI2_DMA_Init(void);
void SPI2_Write(uint8_t data);
void SPI2_Write_DMA(uint8_t *data, uint16_t len);
void SPI2_SetDataSize(uint16_t dataSize);
void SPI2_Write_DMA16(uint16_t *data, uint16_t count);

#endif
