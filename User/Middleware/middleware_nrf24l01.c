/**
 * @file    middleware_nrf24l01.c
 * @brief   NRF24L01 协议中间件：小车状态数据的打包与发送
 */
#include "middleware_nrf24l01.h"
#include "driver_nrf24l01.h"
#include "task_nrf24l01.h"
#include "task_control.h"

/**
 * @brief   无线协议初始化（占位）
 * @retval  None
 */
void middleware_nrf24l01_init(void)
{

}

/**
 * @brief   将小车状态数据打包进发送缓冲区并发送
 * @note    数据包布局（共 16 字节）：
 *           [0] 数据包类型，[1:2] 左右轮目标速度，
 *           [4:7] 角度(float)，[8:11] 左轮实际速度(float)，
 *           [12:15] 右轮实际速度(float)（4 字节对齐，故从 [4] 开始）
 * @param   car_data  小车状态数据
 * @retval  None
 */
void middleware_nrf24l01_send_packet(middleware_nrf24l01_car_data_t *car_data)
{
    driver_nrf24l01_tx_packet[0] = TASK_NRF24L01_CAR_DATA;
    driver_nrf24l01_tx_packet[1] = car_data->left_speed;
    driver_nrf24l01_tx_packet[2] = car_data->right_speed;
    // 4、5、6、7 为角度，为字节对齐从 4 开始
    *(float *)&driver_nrf24l01_tx_packet[4] = car_data->angle;
    // 8、9、10、11 为左轮实际速度
    *(float *)&driver_nrf24l01_tx_packet[8] = car_data->left_actual_speed;
    // 12、13、14、15 为右轮实际速度
    *(float *)&driver_nrf24l01_tx_packet[12] = car_data->right_actual_speed;
    driver_nrf24l01_send_packet();
}
