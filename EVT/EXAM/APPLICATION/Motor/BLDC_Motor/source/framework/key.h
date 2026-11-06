/********************************** (C) COPYRIGHT  *******************************
 * File Name          : key.h
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
#ifndef KEY_H
#define KEY_H

#include "tztype.h"

// Debounce time, unit: ms
#define KEY_DEBOUNCE_TIME 20
// Default minimum long-press time, unit: ms
#define KEY_LONG_PUSH_TIME_MIN_DEFAULT 2000
// Default minimum extra-long-press time, unit: ms
#define KEY_EXTRA_LONG_PUSH_TIME_MIN_DEFAULT 10000

// KeyEvent: key event
typedef enum {
    // Key released
    KEY_EVENT_RELEASE,
    // Generated when the key is pressed, no need to wait for release
    KEY_EVENT_PUSH,
    // Generated when the key is released
    KEY_EVENT_SHORT_PUSH,
    // Generated on a long press
    KEY_EVENT_LONG_PUSH,
    // Generated on an extra-long press. The long-press event is generated before this event
    KEY_EVENT_EXTRA_LONG_PUSH
} KeyEvent;

// KeyGetStateFunc: callback type that gets the key state
typedef bool (*KeyGetStateFunc)(void);

// KeyPushEventFunc: key event callback type
typedef void (*KeyPushEventFunc)(int key, KeyEvent event);

// KeyLoad: module load
bool KeyLoad(uint8_t *buf, int len);

// KeyRun: key run
void KeyRun(void);

// KeyRegister: register a key
bool KeyRegister(int key, KeyGetStateFunc getKeyState);

// KeyRegisterObserver: register a key event observer
bool KeyRegisterObserver(KeyPushEventFunc callback);

// KeySetLongPushTime: set the long-press event duration, unit: s
void KeySetLongPushTime(int time);

// KeySetLongPushTime: set the extra-long-press event duration, unit: s
void KeySetExtraLongPushTime(int time);

// KeySetDebounceTime: set the debounce time, unit: ms
void KeySetDebounceTime(int time);

// KeyGetState: get the key state
bool KeyGetState(int key);

#endif // KEY_H
