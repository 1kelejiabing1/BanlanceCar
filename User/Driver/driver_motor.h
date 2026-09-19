/**
 * @file    driver_motor.h
 * @brief   电机驱动接口声明（PWM 调速 + 方向控制）
 */
#ifndef DRIVER_MOTOR_H
#define DRIVER_MOTOR_H

#include "main.h"
#include <stdint.h>

void driver_motor_init(void);

/* 开启 / 关闭电机 */
void driver_motor_start(void);
void driver_motor_stop(void);

/* 设置左右电机速度（范围 -100 ~ +100） */
void driver_motor_set_left_speed(int8_t speed);
void driver_motor_set_right_speed(int8_t speed);

#endif
