/**
 * @file    task_nrf24l01.c
 * @brief   NRF24L01 无线收发任务：接收遥控数据、回传小车状态
 *
 * 周期查询 NRF24L01 是否收到数据包，收到后解析摇杆数据控制小车；
 * 当工作模式为"发送 + 回传"时，采集小车状态数据回传给遥控端。
 */
#include "task_nrf24l01.h"
#include "FreeRTOS.h"
#include "task.h"
#include "driver_nrf24l01.h"
#include "middleware_nrf24l01.h"
#include "task_control.h"
#include "app_debug.h"

/* 当前工作模式：0=仅发送、1=发送 + 回传（由遥控端下发的数据包第 0 字节决定） */
task_nrf24l01_mode_t task_nrf24l01_mode;
static middleware_nrf24l01_joystick_data_t task_nrf24l01_joystick_data;
static middleware_nrf24l01_car_data_t task_nrf24l01_car_data;

/**
 * @brief   NRF24L01 无线收发任务主体
 * @param   pvParameters  未使用
 * @retval  None
 */
void task_nrf24l01(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while(1)
    {
        /* 收到遥控数据包：解析摇杆数据并控制小车 */
        if(driver_nrf24l01_receive_packet() == 1)
        {
            task_nrf24l01_mode = driver_nrf24l01_rx_packet[0];
            task_nrf24l01_joystick_data.left_horizontal = driver_nrf24l01_rx_packet[1];
            task_nrf24l01_joystick_data.left_vertical = driver_nrf24l01_rx_packet[2];
            task_nrf24l01_joystick_data.right_horizontal = driver_nrf24l01_rx_packet[3];
            task_nrf24l01_joystick_data.right_vertical = driver_nrf24l01_rx_packet[4];
            task_control_nrf24l01_control(&task_nrf24l01_joystick_data);
        }
        /* 回传模式：采集小车状态数据回传给遥控端 */
        if(task_nrf24l01_mode == TASK_NRF24L01_SEND_RECEIVE)
        {
            task_control_nrf24l01_get_data(&task_nrf24l01_car_data);
            middleware_nrf24l01_send_packet(&task_nrf24l01_car_data);
        }
        UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
        debug_printf("%d\r\n", stackLeft);
        vTaskDelayUntil(&xLastWakeTime, 10);
    }
}


