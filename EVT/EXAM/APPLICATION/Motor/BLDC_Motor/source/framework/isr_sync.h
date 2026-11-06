/********************************** (C) COPYRIGHT  *******************************
 * File Name          : isr_sync.h
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
#ifndef ISR_SYNC_H
#define ISR_SYNC_H

#include <stdint.h>

// For naturally aligned 32-bit communication on a single-core CPU only; no atomic read-modify-write requiring the A extension is used.
// Double buffering requires a single low-priority producer and a high-priority consumer, and the consumer must not retain a slot pointer.
__attribute__((always_inline)) static inline uint32_t IsrLoadRelaxed(const volatile uint32_t *value) {
    return __atomic_load_n(value, __ATOMIC_RELAXED);
}

__attribute__((always_inline)) static inline uint32_t IsrLoadAcquire(const volatile uint32_t *value) {
    return __atomic_load_n(value, __ATOMIC_ACQUIRE);
}

__attribute__((always_inline)) static inline void IsrStoreRelaxed(volatile uint32_t *target, uint32_t value) {
    __atomic_store_n(target, value, __ATOMIC_RELAXED);
}

__attribute__((always_inline)) static inline void IsrStoreRelease(volatile uint32_t *target, uint32_t value) {
    __atomic_store_n(target, value, __ATOMIC_RELEASE);
}

__attribute__((always_inline)) static inline void IsrReleaseFence(void) {
    __atomic_thread_fence(__ATOMIC_RELEASE);
}

__attribute__((always_inline)) static inline void IsrAcquireFence(void) {
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
}

#endif
