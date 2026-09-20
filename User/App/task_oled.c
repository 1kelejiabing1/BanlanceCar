/**
 * @file    task_oled.c
 * @brief   OLED 刷新任务
 *
 * 将 OLED 刷新独立成一个任务的原因：
 *   OLED 采用软件模拟 I2C，刷新一屏约需 39ms（阻塞），若放在控制任务中会
 *   挤占控制任务时间，导致其他低优先级任务无法运行。故单独建任务，以 60ms
 *   周期刷新，与控制任务解耦。
 */
#include "task_oled.h"
#include "FreeRTOS.h"
#include "task.h"
#include "app_debug.h"

/**
 * @brief   OLED 任务主体：周期刷新显示
 * @param   pvParameters  未使用
 * @retval  None
 */
void task_oled(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1) {
        task_control_oled_show();      // 绘制状态信息并上屏
        UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
        debug_printf("%d\r\n", stackLeft);
        vTaskDelayUntil(&xLastWakeTime, 60);
        
    }
}
