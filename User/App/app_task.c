/**
 * @file    app_task.c
 * @brief   应用任务创建入口：初始化各驱动模块并创建 FreeRTOS 任务
 *
 * 本文件是用户应用的启动点，由 freertos.c 的 MX_FREERTOS_Init() 调用。
 * 职责：
 *   1. 初始化各外设驱动（OLED / MPU6500 / 蓝牙 / 电机 / 编码器）
 *   2. 按优先级创建各应用任务（LED / 按键 / 控制 / 蓝牙 / 无线 / OLED）
 * 注意：所有 *_init 型初始化必须放在任务创建之前，避免任务使用到尚未创建的
 *       队列 / 信号量等内核对象（否则会触发 HardFault）。
 */
#include "app_task.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "app_debug.h"
#include "task_key.h"
#include "task_mpu6500.h"
#include "task_control.h"
#include "task_led.h"
#include "task_nrf24l01.h"
#include "task_ble.h"
#include "task_middleware_ble.h"
#include "task_oled.h"

/* 各任务的优先级与栈大小（栈大小单位为 word，即 4 字节） */
// LED 心跳任务
#define LED_TASK_PRIORITY 1
#define LED_TASK_STACK_SIZE 8
// OLED 显示任务
#define OLED_TASK_PRIORITY 1
#define OLED_TASK_STACK_SIZE 128
// 按键处理任务
#define KEY_TASK_PRIORITY 2
#define KEY_TASK_STACK_SIZE 128
// 平衡控制任务（三环 PID）
#define MPU6050_TASK_PRIORITY 5
#define MPU6050_TASK_STACK_SIZE 256
// 蓝牙指令处理任务
#define BLE_TASK_PRIORITY 3
#define BLE_TASK_STACK_SIZE 256
// 蓝牙字节解析任务
#define MIDDLEWARE_BLE_PRIORITY 2
#define MIDDLEWARE_BLE_STACK_SIZE 128
// NRF24L01 无线收发任务
#define NRF24L01_TASK_PRIORITY 4
#define NRF24L01_TASK_STACK_SIZE 256

/**
 * @brief   创建所有应用任务
 * @note    在 MX_FREERTOS_Init() 中调用，位于内核启动之前
 * @retval  None
 */
void App_Task_Create(void)
{
    OLED_Init();
    driver_mpu6500_init();
    driver_ble_init();
    middleware_ble_init();
    driver_motor_init();
    driver_encoder_init();
    debug_printf("Initial free heap: %d bytes\r\n", xPortGetFreeHeapSize());
    BaseType_t xResult;
	
    xResult = xTaskCreate(task_led, "task_led", LED_TASK_STACK_SIZE, NULL, LED_TASK_PRIORITY, NULL);
    if(xResult == pdPASS)
    {
        debug_printf("LED任务创建成功\r\n");
    }
    else
    {
        debug_printf("LED任务创建失败\r\n");
    }
    
    // 创建按键处理任务（优先级中等）
    xResult = xTaskCreate(task_key, "key_task", KEY_TASK_STACK_SIZE, NULL, KEY_TASK_PRIORITY, NULL);
    if (xResult == pdPASS)
    {
        debug_printf("按键任务创建成功\r\n");
    }
    else
    {
        debug_printf("按键任务创建失败\r\n");
    }

    xResult = xTaskCreate(task_control, "task_control", MPU6050_TASK_STACK_SIZE, NULL, MPU6050_TASK_PRIORITY, NULL);
    if (xResult == pdPASS)
    {
        debug_printf("task_control任务创建成功\r\n");
    }
    else
    {
        debug_printf("task_control任务创建失败\r\n");
    }

    xResult = xTaskCreate(task_middleware_ble, "task_middleware_ble", MIDDLEWARE_BLE_STACK_SIZE, NULL, MIDDLEWARE_BLE_PRIORITY, NULL);
    if (xResult == pdPASS)
    {
        debug_printf("BLE协议解析任务创建成功\r\n");
    }
    else
    {
        debug_printf("BLE协议解析任务创建失败\r\n");
    }

    xResult = xTaskCreate(task_ble, "task_ble", BLE_TASK_STACK_SIZE, NULL, BLE_TASK_PRIORITY, NULL);
    if (xResult == pdPASS)
    {
        debug_printf("BLE任务创建成功\r\n");
    }
    else
    {
        debug_printf("BLE任务创建失败\r\n");
    }
    xResult = xTaskCreate(task_nrf24l01, "task_nrf24l01", NRF24L01_TASK_STACK_SIZE, NULL, NRF24L01_TASK_PRIORITY, NULL);
    if (xResult == pdPASS)
    {
        debug_printf("NRF24L01任务创建成功\r\n");
    }
    else
    {
        debug_printf("NRF24L01任务创建失败\r\n");
    }
    xResult = xTaskCreate(task_oled, "task_oled", OLED_TASK_STACK_SIZE, NULL, OLED_TASK_PRIORITY, NULL);
    if (xResult == pdPASS)
    {
        debug_printf("OLED任务创建成功\r\n");
    }
    else
    {
        debug_printf("OLED任务创建失败\r\n");
    }
    debug_printf("heap: %d bytes\r\n", xPortGetFreeHeapSize());
}
