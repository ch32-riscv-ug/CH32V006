/********************************** (C) COPYRIGHT  *******************************
 * File Name          : key.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2026/08/05
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

// KeyLoad 按键载入
// buf 内存首地址
// len 内存长度
// keyNum 按键数量
// 长按时间 单位ms
// 超长按时间 单位ms
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

// KeyRun 按键运行
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

    // 等按下
    PT_WAIT_UNTIL(&item->pt, item->keyIO() == true);

    // 防抖
    item->timestamp = TZTimeGetMillsecond();
    PT_WAIT_UNTIL(&item->pt, item->keyIO() == false || TZTimeGetMillsecond() - item->timestamp > gDebounceTime);
    if (item->keyIO() == false) {
        // 存在抖动
        PT_EXIT(&item->pt);
    }
    notifyObserver(item->key, KEY_EVENT_PUSH);

    // 等待松开或者长按事件
    PT_WAIT_UNTIL(&item->pt, item->keyIO() == false || TZTimeGetMillsecond() - item->timestamp > gLongPushTime);
    if (item->keyIO() == false) {
        // 短按释放按键
        notifyObserver(item->key, KEY_EVENT_SHORT_PUSH);
        PT_EXIT(&item->pt);
    }
    notifyObserver(item->key, KEY_EVENT_LONG_PUSH);

    // 等待松开或者超长按事件
    PT_WAIT_UNTIL(&item->pt, item->keyIO() == false || TZTimeGetMillsecond() - item->timestamp > gExtraLongPushTime);
    if (item->keyIO() == false) {
        // 短按释放按键
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

// KeyGetState 读取按键状态
bool KeyGetState(int index) {
    for (int i = 0; i < gItemLen; i++) {
        if (gItem[i].key == index) {
            return gItem[i].keyIO();
        }
    }

    return false;
}

// KeySetLongPushTime 设置长按事件的时长.单位:ms
void KeySetLongPushTime(int time) {
    gLongPushTime = time;
}

// KeySetExtraLongPushTime 设置超长按事件的时长.单位:ms
void KeySetExtraLongPushTime(int time) {
    gExtraLongPushTime = time;
}

// KeySetDebounceTime 设置防抖时间.单位:ms
void KeySetDebounceTime(int time) {
    gDebounceTime = time;
}

// KeyRegister 注册按键
bool KeyRegister(int key, KeyGetStateFunc getKeyState) {
    // 检测内存是否足够
    if (gBufUsedLen + sizeof(tItem) > gBufLen) {
        return false;
    }

    // 检测有没有被重复注册
    for (int i = 0; i < gItemLen; i++) {
        if (gItem[i].key == key) {
            gItem->keyIO = getKeyState;
            return true;
        }
    }

    gItem[gItemLen] = (tItem){.key = key,
                                .keyIO = getKeyState};
    gBufUsedLen += sizeof(tItem);
    gItemLen++;

    return true;
}

// KeyRegisterObserver 注册观察者
bool KeyRegisterObserver(KeyPushEventFunc callback) {
    // 检测内存是否足够
    if (gBufUsedLen + sizeof(KeyPushEventFunc) > gBufLen) {
        return false;
    }

    // 检测有没有被重复注册
    for (int i = 0; i < gObserverFuncLen; i++) {
        if (gObserverFunc[i] == callback) {
            return true;
        }
    }

    // 从末尾开始从后往前添加回调函数
    if (gObserverFuncLen == 0) {
        gObserverFunc = (KeyPushEventFunc *)(gBuf + gBufLen - sizeof(KeyPushEventFunc));
    } else {
        // 该变量是函数指针, +-1的操作是以函数指针的单位进行偏移(4字节)
        gObserverFunc -= 1;
    }

    *gObserverFunc = callback;
    gObserverFuncLen++;
    gBufUsedLen += sizeof(KeyPushEventFunc);

    return true;
}
