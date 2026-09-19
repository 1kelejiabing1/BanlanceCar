/**
 * @file    task_led.c
 * @brief   LED 心跳任务：周期翻转 LED，指示系统正常运行
 */
#include "task_led.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bsp_led.h"

/**
 * @brief   LED 任务主体：每 500ms 翻转一次 LED
 * @param   pvParameters  未使用
 * @retval  None
 */
void task_led(void *pvParameters)
{
    while(1)
    {
        BSP_LED_TOGGLE();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
