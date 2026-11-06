// Copyright WCH, all rights reserved.
// Static pulse injection rotor position detection

#ifndef PULSE_H
#define PULSE_H

#include "def.h"

// Static pulse injection configuration parameters (loaded at run time, replacing the config.h compile-time macros)
typedef struct {
    uint32_t systemClockFreq;    // System clock frequency (Hz)
    uint32_t adcTimeoutUs;       // ADC injected sampling timeout (us)
    uint32_t demagTimeUs;        // Demagnetization time (us)
    uint32_t settleTimeUs;       // Current settling time (us)
    int32_t  injectDuty;         // Injection PWM duty cycle (Q15 per-unit)
    uint16_t phaseLossThreshold; // Phase-loss detection current threshold (raw ADC value)
    uint16_t shortThreshold;     // Short-circuit detection current threshold (raw ADC value)
    void (*adcPause)(void);      // Pause regular-group sampling before injection (e.g. DrvAdcPauseRegular), required
    void (*adcResume)(void);     // Resume regular-group sampling after injection (e.g. DrvAdcResumeRegular), required
} PulseConfig_T;

// Static pulse injection diagnostics; all arrays are ordered by injection sequence AB, BA, AC, CA, BC, CB
// The floating-phase voltage is used to evaluate saturation/saliency signal strength (refer to the LKS08x gun-drill scheme); it does not participate in sector decisions yet:
//   diff[] = floating-phase voltage difference between the forward and reverse injection of the same pair, reflecting saliency (varies with 2*theta)
//   the three-phase difference of the forward+reverse sums reflects magnetic saturation polarity (varies with theta)
typedef struct {
    uint16_t current[6]; // Bus current absolute value (raw ADC value, zero offset already removed)
    uint16_t voltage[6]; // Floating-phase terminal voltage of the same conversion (raw ADC value); the floating phases are W, W, V, V, U, U in order
    int16_t diff[3];     // Floating-phase voltage differences: AB-BA, AC-CA, BC-CB
    uint16_t vbus;       // Most recent bus voltage sample before injection (raw ADC value)
    uint8_t sector;      // Sector determined this time (1-6), 0 means invalid
    uint8_t valid;       // Whether all 6 directions have been sampled
    uint16_t count;      // Cumulative detection count, used to confirm data has been refreshed
} PulseDiag_T;

// Injection-complete flag
extern volatile bool gPulseInjectDone;

// Get the diagnostics of the most recent static pulse injection
const volatile PulseDiag_T *PulseGetDiag(void);

// Static pulse injection initialization
// config: configuration parameter pointer; pass NULL to use the defaults
void PulseLoad(const PulseConfig_T *config);

// Static pulse injection detection
// Called once per PWM period
// Returns true when detection is complete, false otherwise
bool PulseDetect(void);

// Get the detected sector (1-6), 0 means invalid
uint8_t PulseGetSector(void);

// Reset the static pulse injection parameters
void PulseReset(void);

#endif
