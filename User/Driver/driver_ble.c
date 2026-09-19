/**
 * @file    driver_ble.c
 * @brief   蓝牙（透传）驱动：基于 UART 中断接收 + 信号量通知
 *
 * 数据流：USART2 中断收到字节 → 写入 middleware 环形缓冲区 →
 *         释放信号量通知解析任务 → 重新开启接收中断。
 */
#include "driver_ble.h"
#include <string.h>
#include "usart.h"
#include "middleware_ble.h"
#include "stm32f103xb.h"
#include "app_debug.h"

static UART_HandleTypeDef *g_ble_huart = &huart2;   /* 蓝牙串口句柄（USART2） */
static USART_TypeDef *g_ble_uart_reg = USART2;      /* 蓝牙串口外设基地址 */
static uint8_t g_rx_byte;                           /* 单字节接收缓冲 */
xSemaphoreHandle driver_ble_rx_byte_sem;            /* 字节到达信号量 */

/**
 * @brief   蓝牙驱动初始化
 * @note    串口外设初始化由 CubeMX 在 main.c 中完成，这里创建信号量并开启接收中断
 * @retval  None
 */
void driver_ble_init(void)
{
    // 创建二值信号量，用于通知解析任务"有字节到达"
    driver_ble_rx_byte_sem = xSemaphoreCreateBinary();
    HAL_UART_Receive_IT(g_ble_huart, (uint8_t *)&g_rx_byte, 1);
	debug_printf("Bluetooth Init OK\r\n");
}

/**
 * @brief   通过蓝牙发送数据块
 * @param   buf   数据缓冲区
 * @param   size  数据长度
 * @retval  None
 */
void driver_ble_send_buf(uint8_t *buf, uint16_t size)
{
    HAL_UART_Transmit(g_ble_huart, buf, size, portMAX_DELAY);
}

/**
 * @brief   UART 接收完成回调（中断上下文）
 * @param   huart  UART 句柄
 * @retval  None
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (huart->Instance == g_ble_uart_reg)
    {
        // 接收到数据，写入协议层环形缓冲区
        middleware_ble_write_byte(g_rx_byte);
        // 释放信号量，唤醒解析任务
        xSemaphoreGiveFromISR(driver_ble_rx_byte_sem, &xHigherPriorityTaskWoken);
        // 重新开启接收中断
        HAL_UART_Receive_IT(huart, (uint8_t *)&g_rx_byte, 1);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}
