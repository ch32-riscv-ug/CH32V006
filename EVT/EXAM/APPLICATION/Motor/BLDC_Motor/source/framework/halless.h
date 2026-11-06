// Copyright WCH, all rights reserved.
// Sensorless observer module

#ifndef HALLESS_H
#define HALLESS_H

#include "stddef.h"
#include "stdint.h"
#include "stdbool.h"

// HallessLoad: load
// zeroRatio: zero-crossing ratio, range 0-100, recommended 5
// supportHighSpeed: whether high speed is supported; when supported, filtering is disabled automatically at high speed
void HallessLoad(uint8_t zeroRatio, uint32_t supportHighSpeed);

// CheckZeroCross: detect a zero crossing
// dir 0: forward  1: reverse
bool CheckZeroCross(uint32_t dir, uint32_t phase, uint16_t bemfNow, uint16_t bemfRef);

// CheckZeroCrossMajority: majority-filtered zero-crossing detection
// Same interface as CheckZeroCross; internally uses the ADC_BEMF_FILTER majority-filter state machine
// dir 0: forward  1: reverse
bool CheckZeroCrossMajority(uint32_t dir, uint32_t phase, uint16_t bemfNow, uint16_t zeroValue);

// CheckZeroCrossSimple: simplified zero-crossing version
// Same interface as CheckZeroCross; internally uses the ADC_BEMF_FILTER majority-filter state machine
// dir 0: forward  1: reverse
__attribute__((section(".highcode"))) bool CheckZeroCrossSimple(uint32_t dir, uint32_t phase, uint16_t bemfNow, uint16_t zeroValue);

// HallessReset: reset the sensorless parameters
void HallessReset(void);

#endif
