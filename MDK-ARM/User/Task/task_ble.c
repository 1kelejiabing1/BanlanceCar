/**
 * @file    task_ble.c
 * @brief   蓝牙任务实现（FreeRTOS原生API）
 */

#include "task_ble.h"
#include "FreeRTOS.h"
#include "app_debug.h"
#include "ble_protocol.h"
#include "ble_uart.h"
#include "task.h"

void BLE_DataCallback(uint8_t *data, uint16_t len);

static UART_HandleTypeDef *g_huart = NULL;
static TaskHandle_t g_ble_task_handle = NULL;

// 蓝牙数据接收回调
void BLE_DataCallback(uint8_t *data, uint16_t len)
{
    debug_printf("收到蓝牙数据：\r\n");
    for (int i = 0; i < len; i++)
    {
        debug_printf("%02X ", data[i]);
    }
    debug_printf("\r\n");
    debug_printf("字符串：%.*s\r\n", len, data);
}

/**
 * @brief   蓝牙接收任务
 */
static void BLE_RxTask(void *pvParameters)
{
    /* 等待系统稳定 */
    vTaskDelay(pdMS_TO_TICKS(100));

    /* 初始化 */
    BLE_UART_Init(g_huart);
    Protocol_Init();

    uint8_t rx_byte;

    Protocol_RegisterCallback(BLE_DataCallback);
    while (1)
    {
        /* 从环形缓冲区读取数据并解析 */
        while (BLE_UART_GetChar(&rx_byte))
        {
            Protocol_Parse(rx_byte);
        }
        uint32_t min_stack_free = uxTaskGetStackHighWaterMark(NULL);
        //debug_printf("BLE_RxTask 剩余栈：%d\r\n", min_stack_free);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * @brief   蓝牙发送任务
 */
static void BLE_TxTask(void *pvParameters)
{
    while (1)
    {
        /* 可以在这里定期发送状态数据 */
        /* Protocol_SendFrameFormat("status,%.2f,%.2f", angle, speed); */
		
        uint32_t Task_MinStackFree = uxTaskGetStackHighWaterMark(NULL);
		static uint8_t count = 0;
		count++;
		if(count >= 10)
		{
			count = 0;
			Protocol_SendFrameFormat("Hello");
			HAL_UART_Transmit(g_huart, (uint8_t *)"BLE_TxTask HelloBlueTooth\r\n", 28, 100);
		}
	   //debug_printf("BLE_TxTask 剩余栈：%d\r\n", Task_MinStackFree);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * @brief   创建蓝牙任务
 * @param   huart   UART句柄
 */
void Task_BLE_Init(UART_HandleTypeDef *huart)
{
    g_huart = huart;
    BaseType_t xResult;
    /* 创建任务 */
    xResult = xTaskCreate(
        BLE_RxTask,  /* 任务函数 */
        "BLE_RxTask",        /* 任务名称 */
        1024,              /* 任务栈大小（字节） */
        NULL,              /* 任务参数 */
        3,                 /* 任务优先级 */
        &g_ble_task_handle /* 任务句柄 */
    );
    if (xResult == pdPASS)
    {
        debug_printf("创建蓝牙接收任务成功\r\n");
    }
    else
    {
        debug_printf("创建蓝牙接收任务失败\r\n");
    }

    /* 创建发送任务（可选） */
    xResult = xTaskCreate(
        BLE_TxTask, /* 任务函数 */
        "BLE_TxTask", /* 任务名称 */
        512,        /* 任务栈大小（字节） */
        NULL,       /* 任务参数 */
        2,          /* 任务优先级 */
        NULL        /* 任务句柄 */
    );
    if (xResult == pdPASS)
    {
        debug_printf("创建蓝牙发送任务成功\r\n");
    }
    else
    {
        debug_printf("创建蓝牙发送任务失败\r\n");
    }
}