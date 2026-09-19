/**
 * @file    middleware_ble.c
 * @brief   蓝牙协议中间件：字节流 → 协议帧
 *
 * 协议帧格式：以 '[' 开头、']' 结尾，帧内字段用 ',' 分隔，例如：
 *   [slider,AngleKp,4.5]
 *   [joystick,0,50,30,0]
 *
 * 实现：
 *   1. 串口字节先经环形缓冲区缓冲（middleware_ble_write_byte / read_byte）
 *   2. 由状态机 middleware_ble_parse_byte 逐字节解析，识别帧头帧尾
 *   3. 解析完成的完整帧写入 FreeRTOS 队列，供应用层消费
 */
#include "middleware_ble.h"
#include "app_debug.h"
#define BLE_PROTOCOL_FRAME_HEADER '['         /* 帧头 */
#define BLE_PROTOCOL_FRAME_TAIL ']'           /* 帧尾 */
#define BLE_PROTOCOL_RX_RINGBUFF_SIZE 128     /* 接收环形缓冲区大小 */

/* 接收环形缓冲区 */
typedef struct
{
    uint8_t buf[BLE_PROTOCOL_RX_RINGBUFF_SIZE];
    uint8_t w_idx;                            /* 写索引 */
    uint8_t r_idx;                            /* 读索引 */
} ble_protocol_rx_ringbuff_t;

/* 解析状态机状态 */
typedef enum
{
    BLE_PROTOCOL_STATE_IDLE = 0u,       /* 空闲：等待帧头 */
    BLE_PROTOCOL_STATE_RECEIVING,       /* 接收中：缓存帧内容 */
    BLE_PROTOCOL_STATE_COMPLETE,        /* 完成：收到帧尾，投递队列 */
    BLE_PROTOCOL_STATE_ERROR,           /* 错误：帧超长，丢弃 */
} ble_protocol_state_t;

QueueHandle_t middleware_bleframe_queue = NULL;   /* 完整帧队列 */
static ble_protocol_rx_ringbuff_t s_ble_protocol_rx_ringbuff;
static ble_protocol_state_t s_ble_protocol_state = BLE_PROTOCOL_STATE_IDLE;
static ble_protocol_frame_t s_ble_protocol_frame;

/**
 * @brief   蓝牙协议中间件初始化：创建帧队列
 * @retval  None
 */
void middleware_ble_init(void)
{
    middleware_bleframe_queue = xQueueCreate(10, sizeof(ble_protocol_frame_t));
    debug_printf("MiddlewareBLE Init OK\r\n");
}

/**
 * @brief   逐字节解析协议帧（状态机）
 * @param   byte  待解析的字节
 * @retval  None
 */
void middleware_ble_parse_byte(uint8_t byte)
{
    switch (s_ble_protocol_state)
    {
		case BLE_PROTOCOL_STATE_IDLE:
		{
			if (byte == BLE_PROTOCOL_FRAME_HEADER)
			{
				s_ble_protocol_state = BLE_PROTOCOL_STATE_RECEIVING;
			}
			break;
		}
		case BLE_PROTOCOL_STATE_RECEIVING:
		{
			if (byte == BLE_PROTOCOL_FRAME_TAIL)
			{
				s_ble_protocol_state = BLE_PROTOCOL_STATE_COMPLETE;
			}
			else if (s_ble_protocol_frame.length >= BLE_PROTOCOL_FRAME_MAX_LENGTH)
			{
				s_ble_protocol_state = BLE_PROTOCOL_STATE_ERROR;
			}
			else
			{
				s_ble_protocol_frame.ble_protocol_frame[s_ble_protocol_frame.length++] = byte;
			}

			break;
		}
		case BLE_PROTOCOL_STATE_COMPLETE:
		{
			xQueueSend(middleware_bleframe_queue, &s_ble_protocol_frame, portMAX_DELAY);
			s_ble_protocol_frame.length = 0;
			s_ble_protocol_state = BLE_PROTOCOL_STATE_IDLE;
			break;
		}
		case BLE_PROTOCOL_STATE_ERROR:
		{
			// 报错
			debug_printf("BLE_PROTOCOL_STATE_ERROR\r\n");
			// 恢复状态，继续解析
			s_ble_protocol_state = BLE_PROTOCOL_STATE_IDLE;
			break;
		}
    }
}

/**
 * @brief   向接收环形缓冲区写入一个字节
 * @note    在 UART 中断上下文中调用，未加锁（单字节索引自增在 32 位 MCU 上为原子操作）
 * @param   byte  要写入的字节
 * @retval  None
 */
void middleware_ble_write_byte(uint8_t byte)
{
    if (s_ble_protocol_rx_ringbuff.w_idx + 1 == s_ble_protocol_rx_ringbuff.r_idx)
    {
        // 环形缓冲区满
        debug_printf("BLE_PROTOCOL_RX_RINGBUFF_FULL\r\n");
    }
    else
    {
        s_ble_protocol_rx_ringbuff.buf[s_ble_protocol_rx_ringbuff.w_idx] = byte;
        s_ble_protocol_rx_ringbuff.w_idx = (s_ble_protocol_rx_ringbuff.w_idx + 1) % BLE_PROTOCOL_RX_RINGBUFF_SIZE;
    }
}

/**
 * @brief   从接收环形缓冲区读出一个字节
 * @param   byte  输出：读出的字节
 * @retval  1=成功读出，0=缓冲区空
 */
uint8_t middleware_ble_read_byte(uint8_t *byte)
{
    if (s_ble_protocol_rx_ringbuff.w_idx == s_ble_protocol_rx_ringbuff.r_idx)
    {
        // 环形缓冲区空
        return 0;
    }
    else
    {
        *byte = s_ble_protocol_rx_ringbuff.buf[s_ble_protocol_rx_ringbuff.r_idx];
        s_ble_protocol_rx_ringbuff.r_idx = (s_ble_protocol_rx_ringbuff.r_idx + 1) % BLE_PROTOCOL_RX_RINGBUFF_SIZE;
        return 1;
    }
}
