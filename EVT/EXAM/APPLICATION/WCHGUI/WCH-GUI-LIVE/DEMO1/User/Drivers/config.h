/*********************************************************************
 * File Name          : config.h
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2023/8/15
 * Description        : file for Config.
 *********************************************************************************
* Copyright (c) 2023 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#ifndef __CONFIG_H__
#define __CONFIG_H__

/* SPI FLASH and LCD_SCREEN GPIO Definition
 *
 * CH32V006K8U pin map:
 *   SPI1 (shared by LCD and SPI flash): SCK-PC5, MOSI-PC6, MISO-PC7
 *   LCD_BLK-PB0, LCD_DC-PB1, LCD_RST-PB2, LCD_CS-PC0
 *   FLASH_CS-PC3
 *   Touch IIC: SCL-PB5, SDA-PB6
 *   USART1: TX-PD5, RX-PD6
 */
#define FLASH_CS_PORT     GPIOC
#define FLASH_CS_RCC      RCC_PB2Periph_GPIOC
#define FLASH_CS_PIN      (GPIO_Pin_3)

#define LCD_LED_RCC       RCC_PB2Periph_GPIOB
#define LCD_LED_PORT      GPIOB
#define LCD_LED_PIN       (GPIO_Pin_0)

#define LCD_DC_RCC        RCC_PB2Periph_GPIOB
#define LCD_DC_PORT       GPIOB
#define LCD_DC_PIN        (GPIO_Pin_1)

#define LCD_RST_RCC       RCC_PB2Periph_GPIOB
#define LCD_RST_PORT      GPIOB
#define LCD_RST_PIN       (GPIO_Pin_2)

#define LCD_CS_RCC        RCC_PB2Periph_GPIOC
#define LCD_CS_PORT       GPIOC
#define LCD_CS_PIN        (GPIO_Pin_0)

#endif
