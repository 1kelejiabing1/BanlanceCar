/**
 * @file    bsp_key.h
 * @brief   按键驱动接口声明（状态机 + 事件队列）
 */
#ifndef BSP_KEY_H
#define BSP_KEY_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

// 按键ID枚举（根据实际硬件定义）
typedef enum
{
    K1 = 0, // K1
    K2 = 1, // K2
    K3 = 2, // K3
    K4 = 3, // K4
} KeyId_e;

// 按键事件类型
typedef enum
{
    KEY_EVENT_NONE = 0,
    KEY_EVENT_CLICK,          // 单击
    KEY_EVENT_DOUBLE_CLICK,   // 双击
    KEY_EVENT_LONG_PRESS,     // 长按
    KEY_EVENT_LONG_PRESS_HOLD // 长按持续（可用于连发）
} KeyEventType_e;

// 按键事件（包含按键ID和事件类型）
typedef struct
{
    KeyId_e id;
    KeyEventType_e event;
} KeyEvent_t;

// 初始化所有按键（在BSP层调用）
void bsp_key_init(void);

// 按键扫描函数（需要周期性调用）
void bsp_key_scan(void);

// 获取按键事件（非阻塞，立即返回）
bool bsp_key_get_event(KeyEvent_t *event);

#endif
