/********************************** (C) COPYRIGHT  *******************************
 * File Name          : key.c
 * Author             : WCH
 * Version            : V1.0.1
 * Date               : 2026/11/05
 * Description        : This file provides all firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "key.h"
#include "pt.h"
#include "tztime.h"

#define TAG "key"

#pragma pack(1)

typedef struct {
    int key;
    uint64_t timestamp;
    struct pt pt;
    TZReadIO keyIO;
} tItem;

#pragma pack()

static uint8_t *gBuf = NULL;
static int gBufLen = 0;
static int gBufUsedLen = 0;

static tItem *gItem = NULL;
static int gItemLen = 0;

static int gLongPushTime = KEY_LONG_PUSH_TIME_MIN_DEFAULT;
static int gExtraLongPushTime = KEY_EXTRA_LONG_PUSH_TIME_MIN_DEFAULT;
static int gDebounceTime = KEY_DEBOUNCE_TIME;

static KeyPushEventFunc *gObserverFunc = NULL;
static int gObserverFuncLen = 0;

static bool gIsLoad = false;

static void notifyObserver(int index, KeyEvent event);
static int task(tItem *item);

// KeyLoad: key load
// buf: memory start address
// len: memory length
// keyNum: number of keys
// long-press time, unit: ms
// extra-long-press time, unit: ms
bool KeyLoad(uint8_t *buf, int len) {
    gBuf = buf;
    gBufLen = len;
    gBufUsedLen = 0;

    gItem = (tItem *)gBuf;
    gItemLen = 0;

    memset(gBuf, 0, gBufLen);

    gIsLoad = true;

    return true;
}

// KeyRun: key run
void KeyRun(void) {
    if (gIsLoad == false) {
        return;
    }

    for (int i = 0; i < gItemLen; i++) {
        task(&gItem[i]);
    }
}

static int task(tItem *item) {
    PT_BEGIN(&item->pt);

    // Wait for press
    PT_WAIT_UNTIL(&item->pt, item->keyIO() == true);

    // Debounce
    item->timestamp = TZTimeGetMillsecond();
    PT_WAIT_UNTIL(&item->pt, item->keyIO() == false || TZTimeGetMillsecond() - item->timestamp > gDebounceTime);
    if (item->keyIO() == false) {
        // Jitter present
        PT_EXIT(&item->pt);
    }
    notifyObserver(item->key, KEY_EVENT_PUSH);

    // Wait for release or the long-press event
    PT_WAIT_UNTIL(&item->pt, item->keyIO() == false || TZTimeGetMillsecond() - item->timestamp > gLongPushTime);
    if (item->keyIO() == false) {
        // Short press, key released
        notifyObserver(item->key, KEY_EVENT_SHORT_PUSH);
        PT_EXIT(&item->pt);
    }
    notifyObserver(item->key, KEY_EVENT_LONG_PUSH);

    // Wait for release or the extra-long-press event
    PT_WAIT_UNTIL(&item->pt, item->keyIO() == false || TZTimeGetMillsecond() - item->timestamp > gExtraLongPushTime);
    if (item->keyIO() == false) {
        // Key released
        notifyObserver(item->key, KEY_EVENT_RELEASE);
        PT_EXIT(&item->pt);
    }

    notifyObserver(item->key, KEY_EVENT_EXTRA_LONG_PUSH);

    PT_WAIT_UNTIL(&item->pt, item->keyIO() == false);
    notifyObserver(item->key, KEY_EVENT_RELEASE);

    PT_END(&item->pt);
}

static void notifyObserver(int index, KeyEvent event) {
    for (int i = 0; i < gObserverFuncLen; i++) {
        if (gObserverFunc[i] != NULL) {
            gObserverFunc[i](index, event);
        }
    }
}

// KeyGetState: read the key state
bool KeyGetState(int index) {
    for (int i = 0; i < gItemLen; i++) {
        if (gItem[i].key == index) {
            return gItem[i].keyIO();
        }
    }

    return false;
}

// KeySetLongPushTime: set the long-press event duration, unit: ms
void KeySetLongPushTime(int time) {
    gLongPushTime = time;
}

// KeySetExtraLongPushTime: set the extra-long-press event duration, unit: ms
void KeySetExtraLongPushTime(int time) {
    gExtraLongPushTime = time;
}

// KeySetDebounceTime: set the debounce time, unit: ms
void KeySetDebounceTime(int time) {
    gDebounceTime = time;
}

// KeyRegister: register a key
bool KeyRegister(int key, KeyGetStateFunc getKeyState) {
    // Check whether memory is sufficient
    if (gBufUsedLen + sizeof(tItem) > gBufLen) {
        return false;
    }

    // Check whether it is already registered
    for (int i = 0; i < gItemLen; i++) {
        if (gItem[i].key == key) {
            gItem[i].keyIO = getKeyState;
            return true;
        }
    }

    gItem[gItemLen] = (tItem){.key = key,
                                .keyIO = getKeyState};
    gBufUsedLen += sizeof(tItem);
    gItemLen++;

    return true;
}

// KeyRegisterObserver: register an observer
bool KeyRegisterObserver(KeyPushEventFunc callback) {
    // Check whether memory is sufficient
    if (gBufUsedLen + sizeof(KeyPushEventFunc) > gBufLen) {
        return false;
    }

    // Check whether it is already registered
    for (int i = 0; i < gObserverFuncLen; i++) {
        if (gObserverFunc[i] == callback) {
            return true;
        }
    }

    // Add callbacks from the end backwards
    if (gObserverFuncLen == 0) {
        gObserverFunc = (KeyPushEventFunc *)(gBuf + gBufLen - sizeof(KeyPushEventFunc));
    } else {
        // This variable is a function pointer, so +-1 offsets by the size of a function pointer (4 bytes)
        gObserverFunc -= 1;
    }

    *gObserverFunc = callback;
    gObserverFuncLen++;
    gBufUsedLen += sizeof(KeyPushEventFunc);

    return true;
}
