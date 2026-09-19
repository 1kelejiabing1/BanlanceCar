/**
 * @file    bsp_uart.c
 * @brief   串口收发任务（预留 / 测试用）
 *
 * 说明：本模块为早期串口调试任务，演示了"中断接收 + 队列转发"的标准用法，
 * 当前应用层未创建该任务（见 App_Task_Create）。
 */
#include "bsp_uart.h"
#include <stdint.h>
#include "usart.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Queue.h"
#include "app_debug.h"
#include "bsp_led.h"

uint8_t g_uart1_received;
QueueHandle_t g_uart1_queue;

/**
 * @brief   串口收发任务：中断收到的字节经队列转发后回显
 * @retval  None
 */
void bsp_uart_task(void)
{
    TickType_t xLastWakeTime;
    g_uart1_queue = xQueueCreate(32, sizeof(uint8_t));
    uint8_t receive_byte;
    // 启动串口中断接收
    HAL_StatusTypeDef state = HAL_UART_Receive_IT(&huart1, &g_uart1_received, 1);
    if (state != HAL_OK)
    {
        debug_printf("UART_Receive_IT Error");
    }
    else
    {
        debug_printf("UART_Receive_IT OK");
    }

    while (1)
    {
        xLastWakeTime = xTaskGetTickCount();
        if (xQueueReceive(g_uart1_queue, &receive_byte, portMAX_DELAY) == pdTRUE)
        {
            // 处理接收到的数据
            HAL_UART_Transmit(&huart1, &receive_byte, 1, HAL_MAX_DELAY);
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
    }
}

/**
 * @brief   UART 接收完成回调（中断上下文）
 * @note    将收到的字节经队列发送给任务处理，并重新开启下一次接收中断
 * @param   huart  UART 句柄
 * @retval  None
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(g_uart1_queue, &g_uart1_received, &xHigherPriorityTaskWoken);

        HAL_UART_Receive_IT(&huart1, &g_uart1_received, 1);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/**
 * @brief   UART 错误回调（中断上下文）
 * @note    清除溢出错误标志后重新开启接收中断，保证串口可继续工作
 * @param   huart  UART 句柄
 * @retval  None
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        // 处理UART错误
        // 清除错误标志
        __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_ORE);
        // 重新启动UART接收中断
        HAL_UART_Receive_IT(&huart1, &g_uart1_received, 1);
    }
}
