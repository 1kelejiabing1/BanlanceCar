/**
 * @file    task_key.h
 * @brief   按键处理任务接口声明
 */
#ifndef TASK_KEY_H
#define TASK_KEY_H

#include "FreeRTOS.h"
#include "queue.h"

/**
 * @brief   获取按键事件队列句柄（供其他任务使用）
 * @retval  队列句柄，未创建时为 NULL
 */
QueueHandle_t task_key_get_event_queue(void);

void task_key(void *pvParameters);
#endif
