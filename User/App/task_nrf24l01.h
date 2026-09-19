/**
 * @file    task_nrf24l01.h
 * @brief   NRF24L01 无线收发任务接口声明
 */
#ifndef TASK_NRF24L01_H
#define TASK_NRF24L01_H

#include "main.h"
#include <stdint.h>
#include "driver_nrf24l01.h"

/**
 * @brief   无线工作模式
 */
typedef enum
{
    TASK_NRF24L01_ONLY_SEND = 0u,   /* 仅发送（不回传） */
    TASK_NRF24L01_SEND_RECEIVE,     /* 发送 + 回传 */
    TASK_NRF24L01_CAR_DATA,         /* 小车数据包类型 */
}task_nrf24l01_mode_t;

extern task_nrf24l01_mode_t task_nrf24l01_mode;
void task_nrf24l01(void *pvParameters);

#endif