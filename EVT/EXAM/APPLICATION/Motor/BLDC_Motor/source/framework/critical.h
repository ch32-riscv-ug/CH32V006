/********************************** (C) COPYRIGHT  *******************************
 * File Name          : critical.h
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
// Temporarily disables global interrupts by saving/restoring the global interrupt enable CSR, protecting atomic access to shared state/peripheral registers

#ifndef CRITICAL_H
#define CRITICAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Global interrupt CSR selection: the CSR holding the global interrupt enable bit differs between WCH QingKe cores
 * - V00x (V002/V004/V005/V006) / M007 : standard mstatus
 * - V20x / V30x / M030 and other newer cores : custom gintsr (0x800), no mstatus
 * Selected automatically from the model macros defined in the chip header; newer cores use gintsr by default.
 * It can also be forced by defining CRITICAL_USE_MSTATUS=0/1 in def.h / config.h. */
#ifndef CRITICAL_USE_MSTATUS
    #if defined(CH32V002) || defined(CH32V004) || defined(CH32V005) || \
        defined(CH32V006) || defined(CH32V007_M007)
        #define CRITICAL_USE_MSTATUS 1
    #else
        #define CRITICAL_USE_MSTATUS 0
    #endif
#endif

#if CRITICAL_USE_MSTATUS
    #define CRITICAL_CSR_STR "mstatus"
#else
    #define CRITICAL_CSR_STR "0x800"
#endif

// Internal use: operate the global interrupt CSR directly, without depending on specific function names in core_riscv.h,
// so it can be used the same way in any WCH RISC-V project.
__attribute__((always_inline)) static inline uint32_t criticalGintsrGet(void)
{
    uint32_t result;
    __asm volatile ("csrr %0, " CRITICAL_CSR_STR : "=r"(result) : : "memory");
    return result;
}

__attribute__((always_inline)) static inline void criticalGintsrSet(uint32_t value)
{
    __asm volatile ("csrw " CRITICAL_CSR_STR ", %0" : : "r"(value) : "memory");
}

// Consistent with the WCH SDK __disable_irq() behavior:
// csrc <csr>, 0x88 clears MIE (bit 3) and MPIE (bit 7), then executes fence.i
__attribute__((always_inline)) static inline void criticalIrqDisable(void)
{
    __asm volatile ("csrc " CRITICAL_CSR_STR ", %0" : : "r"(0x88) : "memory");
    __asm volatile ("fence.i" : : : "memory");
}

/**
 * @brief  Enter a critical section
 * @return The interrupt enable CSR value before entry, used by criticalExit to restore
 * @note   This function only disables global interrupts (MIE); it does not clear any peripheral interrupt flag or PFIC pending.
 *         Callers must pair it with criticalExit, otherwise interrupts stay disabled permanently.
 */
__attribute__((always_inline)) static inline uint32_t criticalEnter(void)
{
    uint32_t gintsr = criticalGintsrGet();
    criticalIrqDisable();
    return gintsr;
}

/**
 * @brief  Exit a critical section
 * @param  gintsr CSR value returned by criticalEnter
 */
__attribute__((always_inline)) static inline void criticalExit(uint32_t gintsr)
{
    criticalGintsrSet(gintsr);
}

/* Internal use: helper macros generating a unique local variable name from __LINE__ to avoid name clashes when nested */
#define CRITICAL_PRIV_CONCAT(a, b) a##b
#define CRITICAL_PRIV_TOKEN(a, b)  CRITICAL_PRIV_CONCAT(a, b)

/**
 * @brief  Critical section code block macro
 * @param  code Code to be protected by the critical section (a single statement or a compound statement { ... })
 * @note   1. The macro declares a local variable in the current scope to hold gintsr, so nesting is supported.
 *         2. code must not contain return / break / continue / goto, otherwise criticalExit is not executed.
 *         3. For complex control flow inside a critical section, use criticalEnter / criticalExit directly.
 *
 *  Example:
 *    CRITICAL_SECTION(applyPhaseRegs());
 *    CRITICAL_SECTION({
 *        applyPhaseRegs();
 *        HoldParm.PhaseIsChange = true;
 *    });
 */
#define CRITICAL_SECTION(code) do { \
    uint32_t CRITICAL_PRIV_TOKEN(_critical_gintsr_save_, __LINE__) = criticalEnter(); \
    code; \
    criticalExit(CRITICAL_PRIV_TOKEN(_critical_gintsr_save_, __LINE__)); \
} while (0)

#ifdef __cplusplus
}
#endif

#endif
