/**
 * @file    bsp_led.h
 * @brief   LED 板级支持：LED 亮 / 灭 / 翻转宏定义
 * @note    LED 低电平点亮（GPIO_PIN_RESET），需根据实际硬件确认
 */
#ifndef BSP_LED_H
#define BSP_LED_H

#include "main.h"

#define BSP_LED_ON() HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET)
#define BSP_LED_OFF() HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET)
#define BSP_LED_TOGGLE() HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin)

#endif
