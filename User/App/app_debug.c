/**
 * @file    app_debug.c
 * @brief   调试输出支持：重定向 printf 到串口
 *
 * 通过重写 C 标准库的 fputc()，使 printf / debug_printf / ble_printf
 * 最终都通过 DEBUG_UART 串口输出。
 * 注意：多个任务同时打印会互相穿插导致乱码，调试时应避免并发打印。
 */
#include "app_debug.h"

#include "usart.h"
#include <stdio.h>
#include <stdint.h>

/* 调试输出使用的串口句柄（当前为 USART2，与蓝牙共用） */
#define DEBUG_UART huart1

/**
 * @brief   重定向 printf 的底层输出：单字节阻塞发送
 * @param   ch  要输出的字符
 * @param   f   文件指针（未使用）
 * @retval  返回输出的字符
 */
int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
