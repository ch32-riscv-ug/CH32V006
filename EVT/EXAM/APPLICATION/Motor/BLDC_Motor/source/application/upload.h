/********************************** (C) COPYRIGHT  *******************************
 * File Name          : upload.h
 * Author             : WCH
 * Version            : V1.0.1
 * Date               : 2026/11/05
 * Description        : This file contains all the functions prototypes for the
 *                       firmware library.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef UPLOAD_H
#define UPLOAD_H

#include "drivers.h"

// UploadSend: legacy protocol upload (4 floats + frame tail, compatible with the existing host tool)
void UploadSend(float data1, float data2,float data3,float data4);

// VScopeSend: virtual oscilloscope protocol upload (AA55 frame + heartbeat, one sample point per call)
void VScopeSend(float data1, float data2, float data3, float data4);

#endif

