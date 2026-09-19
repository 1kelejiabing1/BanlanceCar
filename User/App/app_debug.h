/**
 * @file    app_debug.h
 * @brief   调试输出开关与宏定义
 *
 * 通过 DEBUG_ENABLED 选择调试输出通道：
 *   1 = 串口输出（debug_printf 生效）
 *   2 = 蓝牙输出（ble_printf 生效，默认）
 *   其他 = 关闭所有输出
 */
#ifndef APP_DEBUG_H
#define APP_DEBUG_H

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#define DEBUG_ENABLED 2
/* 模式1：串口调试输出（debug_printf 生效，ble_printf 关闭） */
#if (DEBUG_ENABLED == 1)
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 \
                                                                                                     : __FILE__)
#define debug_printf(format, ...) printf("[%s:%d] " format, __FILENAME__, __LINE__, ##__VA_ARGS__)
#define PRINT_F2(v)  (int)(v), (int)((v)*100)%100
#define ble_printf(format, ...)

/* 模式2：蓝牙输出（ble_printf 生效，debug_printf 关闭，默认） */
#elif (DEBUG_ENABLED == 2)
#define debug_printf(format, ...)
#define PRINT_F2(v) (int)(v), (int)((v)*100)%100
#define ble_printf(format, ...) printf(format, ##__VA_ARGS__)

/* 模式3：关闭所有调试输出 */
#else // DEBUG_ENABLED
#define debug_printf(format, ...)
#define PRINT_F2(v)
#define ble_printf(format, ...)
#endif

#endif
