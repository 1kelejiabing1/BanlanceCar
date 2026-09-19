/**
 * @file    driver_ble.h
 * @brief   蓝牙（透传）驱动接口声明
 */
#ifndef DRIVER_BLUETOOTH_H
#define DRIVER_BLUETOOTH_H

#include "main.h"
#include "stdint.h"
#include "FreeRTOS.h"
#include "semphr.h"

extern xSemaphoreHandle driver_ble_rx_byte_sem;   /* 字节到达信号量 */

void driver_ble_init(void);
void driver_ble_send_buf(uint8_t *buf, uint16_t size);
#endif
