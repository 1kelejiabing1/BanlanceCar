/**
 * @file    bsp_uart.h
 * @brief   串口板级支持接口声明（预留）
 */
#ifndef BSP_UART_H
#define BSP_UART_H

#define UART_RX_BUFFER_SIZE 128
#define UART_TX_BUFFER_SIZE 128

void bsp_uart_task(void);

#endif
