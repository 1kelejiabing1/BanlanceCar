/**
 * @file    task_key.c
 * @brief   按键处理任务：周期扫描按键并分发处理事件
 *
 * 职责：
 *   1. 周期调用 bsp_key_scan() 扫描按键状态机
 *   2. 获取按键事件后直接处理（启动/停止平衡、手动调速等）
 */
#include "task_key.h"
#include "FreeRTOS.h"
#include "app_debug.h"
#include "bsp_key.h"
#include "queue.h"
#include "task.h"
#include "i2c.h"
#include "driver_motor.h"
#include "bsp_led.h"
#include "task_control.h"

// 事件队列句柄（用于向其他任务分发事件，当前采用直接处理方式）
static QueueHandle_t g_key_event_queue = NULL;

/**
 * @brief   按键任务主体：扫描按键并处理事件
 * @param   pvParameters  未使用
 * @retval  None
 */
void task_key(void *pvParameters)
{
    KeyEvent_t event;
    static uint8_t cnt = 0;
    static int8_t s_speed = 0;
    while (1)
    {
        // 从BSP层获取按键事件
        if (bsp_key_get_event(&event))
        {
            switch (event.id)
            {
            case K1:
                if (event.event == KEY_EVENT_CLICK)
                {
                    debug_printf("K1按下\r\n");
                    /* 切换平衡运行标志（uint8_t 写操作在 32 位 MCU 上为原子操作，无需加锁） */
                    g_task_control_run_flag = (g_task_control_run_flag == 1)?0:1;
                    BSP_LED_ON();
                }
                else if (event.event == KEY_EVENT_DOUBLE_CLICK)
                {
                    debug_printf("K1双击\r\n");
                }
                else if (event.event == KEY_EVENT_LONG_PRESS)
                {
                    debug_printf("K1长按\r\n");
                }
                else if (event.event == KEY_EVENT_LONG_PRESS_HOLD)
                {
                    debug_printf("K1长按HOLD\r\n");
                }
                break;
            case K2:
                if (event.event == KEY_EVENT_CLICK)
                {
                    debug_printf("K2按下\r\n");
                    driver_motor_stop();
                    BSP_LED_OFF();
                }
                else if (event.event == KEY_EVENT_DOUBLE_CLICK)
                {
                    debug_printf("K2双击\r\n");
                }
                else if (event.event == KEY_EVENT_LONG_PRESS)
                {
                    debug_printf("K2长按\r\n");
                }
                else if (event.event == KEY_EVENT_LONG_PRESS_HOLD)
                {
                    debug_printf("K2长按HOLD\r\n");
                }
                break;
            case K3:
                if (event.event == KEY_EVENT_CLICK)
                {
                    debug_printf("K3按下\r\n");
                    s_speed += 10;
                    ble_printf("%d\r\n",s_speed);
                    driver_motor_set_left_speed(s_speed);
                    driver_motor_set_right_speed(s_speed);
                }
                else if (event.event == KEY_EVENT_DOUBLE_CLICK)
                {
                    debug_printf("K3双击\r\n");
                }
                else if (event.event == KEY_EVENT_LONG_PRESS)
                {
                    debug_printf("K3长按\r\n");
                }
                else if (event.event == KEY_EVENT_LONG_PRESS_HOLD)
                {
                    debug_printf("K3长按HOLD\r\n");
                }
                break;
            case K4:
                if (event.event == KEY_EVENT_CLICK)
                {
                    debug_printf("K4按下\r\n");
                    s_speed -= 10;
                    ble_printf("%d\r\n",s_speed);
                    driver_motor_set_left_speed(s_speed);
                    driver_motor_set_right_speed(s_speed);
                }
                else if (event.event == KEY_EVENT_DOUBLE_CLICK)
                {
                    debug_printf("K4双击\r\n");
                }
                else if (event.event == KEY_EVENT_LONG_PRESS)
                {
                    debug_printf("K4长按\r\n");
                }
                else if (event.event == KEY_EVENT_LONG_PRESS_HOLD)
                {
                    debug_printf("K4长按HOLD\r\n");
                }
                break;
            default:
                break;
            }
        }
        cnt++;
        if (cnt % 4 == 0)
        {
            // 20ms扫描一次
            cnt = 0;
            bsp_key_scan(); // 扫描所有按键
        }
        UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
        debug_printf("%d\r\n", stackLeft);
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms检查一次
    }
}

/**
 * @brief   获取按键事件队列句柄（供其他任务使用）
 * @retval  队列句柄，未创建时为 NULL
 */
QueueHandle_t task_key_get_event_queue(void)
{
    return g_key_event_queue;
}
