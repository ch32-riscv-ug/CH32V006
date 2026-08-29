/********************************** (C) COPYRIGHT  *******************************
 * File Name          : key.h
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2026/08/05
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

// 消抖时间 单位ms
#define KEY_DEBOUNCE_TIME 20
// 默认长按最小时间.单位:ms
#define KEY_LONG_PUSH_TIME_MIN_DEFAULT 2000
// 默认超长按最小时间.单位:ms
#define KEY_EXTRA_LONG_PUSH_TIME_MIN_DEFAULT 10000

// KeyEvent 按键事件
typedef enum {
    // 释放按键
    KEY_EVENT_RELEASE,
    // 按键按下时产生本事件,无需等待释放
    KEY_EVENT_PUSH,
    // 按键松开时产生本事件
    KEY_EVENT_SHORT_PUSH,
    // 长按键时产生本事件
    KEY_EVENT_LONG_PUSH,
    // 超长按键时产生本事件.本事件产生前会先产生长按事件
    KEY_EVENT_EXTRA_LONG_PUSH
} KeyEvent;

// KeyGetStateFunc 获取按键状态回调函数类型
typedef bool (*KeyGetStateFunc)(void);

// KeyPushEventFunc 按键事件回调函数类型
typedef void (*KeyPushEventFunc)(int key, KeyEvent event);

// KeyLoad 模块载入
bool KeyLoad(uint8_t *buf, int len);

// KeyRun 按键运行
void KeyRun(void);

// KeyRegister 注册按键
bool KeyRegister(int key, KeyGetStateFunc getKeyState);

// KeyRegisterObserver 注册按键事件观察者
bool KeyRegisterObserver(KeyPushEventFunc callback);

// KeySetLongPushTime 设置长按事件的时长.单位:s
void KeySetLongPushTime(int time);

// KeySetLongPushTime 设置超长按事件的时长.单位:s
void KeySetExtraLongPushTime(int time);

// KeySetDebounceTime 设置防抖时间.单位:ms
void KeySetDebounceTime(int time);

// KeyGetState 获取按键状态
bool KeyGetState(int key);

#endif // KEY_H
