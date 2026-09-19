/**
 * @file    middleware_nrf24l01.h
 * @brief   NRF24L01 协议中间件接口声明
 */
#ifndef MIDDLEWARE_NRF24L01_H
#define MIDDLEWARE_NRF24L01_H

#include "main.h"
#include <stdint.h>

/**
 * @brief   遥控端发送给小车的摇杆数据
 */
typedef struct
{
    int8_t left_horizontal;
    int8_t left_vertical;
    int8_t right_horizontal;
    int8_t right_vertical;
}middleware_nrf24l01_joystick_data_t;

/**
 * @brief   小车回传的速度、角度数据
 */
typedef struct
{
    int8_t left_speed;           /* 左轮目标速度 */
    int8_t right_speed;          /* 右轮目标速度 */
    float angle;                 /* 俯仰角 */
    float left_actual_speed;     /* 左轮实际速度 */
    float right_actual_speed;    /* 右轮实际速度 */
}middleware_nrf24l01_car_data_t;

void middleware_nrf24l01_init(void);

void middleware_nrf24l01_send_packet(middleware_nrf24l01_car_data_t *car_data);
#endif
