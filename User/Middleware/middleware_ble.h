/**
 * @file    middleware_ble.h
 * @brief   蓝牙协议中间件接口声明（字节流 → 协议帧）
 */
#ifndef MIDDLEWARE_BLUETOOTH_PROTOCOL_H
#define MIDDLEWARE_BLUETOOTH_PROTOCOL_H

#include <stdint.h>
#include "main.h"
#include "FreeRTOS.h"
#include "queue.h"

#define BLE_PROTOCOL_FRAME_MAX_LENGTH 32   /* 单帧最大长度（不含帧头帧尾） */

/**
 * @brief   协议帧结构：帧内容 + 有效长度
 */
typedef struct
{
    uint8_t ble_protocol_frame[BLE_PROTOCOL_FRAME_MAX_LENGTH];
    uint8_t length;
} ble_protocol_frame_t;

extern QueueHandle_t middleware_bleframe_queue;   /* 完整帧队列 */

void middleware_ble_init(void);
void middleware_ble_parse_byte(uint8_t byte);
void middleware_ble_write_byte(uint8_t byte);
uint8_t middleware_ble_read_byte(uint8_t *byte);

#endif
