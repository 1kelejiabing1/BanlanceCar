/**
 * @file    task_mpu6500.c
 * @brief   MPU6500 调试任务（早期遗留，当前未使用）
 *
 * 说明：MPU6500 数据读取已整合进 task_control（控制任务），本任务仅为
 * 早期单独调试 MPU6500 数据输出时使用，当前不会在 App_Task_Create 中创建。
 * 如不再需要，可连同 task_mpu6500.h 一并删除。
 */
#include "task_mpu6500.h"
#include "FreeRTOS.h"
#include "task.h"
#include "driver_mpu6500.h"
#include "app_debug.h"

/**
 * @brief   MPU6500 调试任务主体
 * @param   pvParameters  未使用
 * @retval  None
 */
void task_mpu6500(void *pvParameters)
{
    while (1)
    {
        mpu6500_raw_data_t raw;
        mpu6500_scale_data_t scaled;
        if (driver_mpu6500_read_raw(&raw))
        {
            driver_mpu6500_convert(&raw, &scaled);
            ble_printf("[plot,%.2f]", scaled.gyro_y - 0.8);
        }
        else
        {
            debug_printf("[mpu6500] 读取传感器数据失败\r\n");
            ble_printf("[mpu6500] 读取传感器数据失败\r\n");
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
