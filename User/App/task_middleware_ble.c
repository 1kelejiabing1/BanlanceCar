/**
 * @file    task_middleware_ble.c
 * @brief   蓝牙字节解析任务：将串口字节流解析为完整协议帧
 *
 * 驱动层通过信号量通知有字节到达，本任务从环形缓冲区逐字节取出，
 * 交给 middleware_ble_parse_byte() 状态机解析；解析出的完整帧进入
 * 帧队列，由 task_ble 消费。
 */
#include "task_middleware_ble.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "middleware_ble.h"
#include "driver_ble.h"
#include "app_debug.h"

/**
 * @brief   蓝牙字节解析任务主体
 * @param   pvParameters  未使用
 * @retval  None
 */
void task_middleware_ble(void *pvParameters)
{
    while (1)
    {
        // 等待驱动层信号量（有字节到达）
        xSemaphoreTake(driver_ble_rx_byte_sem, portMAX_DELAY);
        uint8_t byte;
        // 从环形缓冲区取出所有已到达字节，逐一喂给协议状态机
        while (middleware_ble_read_byte(&byte))
        {
            middleware_ble_parse_byte(byte);
            UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
            debug_printf("%d\r\n", stackLeft);
        }
    }
}
