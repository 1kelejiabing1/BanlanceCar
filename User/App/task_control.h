/**
 * @file    task_control.h
 * @brief   平衡控制任务接口声明
 */
#ifndef TASK_CONTROL_H
#define TASK_CONTROL_H

#include "main.h"
#include <stdint.h>
#include "middleware_nrf24l01.h"

/**
 * @brief   要修改哪个 PID 环
 */
typedef enum
{
    TASK_CONTROL_ANGLE_PID = 0u,    /* 直立环（角度环） */
    TASK_CONTROL_SPEED_PID,         /* 速度环 */
    TASK_CONTROL_TURN_PID,          /* 转向环 */
} task_control_pid_t;

/**
 * @brief   要修改哪个 PID 参数
 */
typedef enum
{
    TASK_CONTROL_PID_P = 0u,        /* 比例项 Kp */
    TASK_CONTROL_PID_I,             /* 积分项 Ki */
    TASK_CONTROL_PID_D,             /* 微分项 Kd */
    TASK_CONTROL_PID_OFFSET,        /* 输出偏移 */
    TASK_CONTROL_PID_TARGET,        /* 目标值 */
} task_control_pid_param_t;

/* 平衡控制运行标志：1=运行，0=停止（在 32 位 MCU 上 uint8_t 写操作为原子操作，无需加锁） */
extern volatile uint8_t g_task_control_run_flag;

void task_control(void *pvParameters);

void task_control_oled_show(void);

void task_control_set_pid_param(task_control_pid_t pid, task_control_pid_param_t param_mode, float value);

void task_control_nrf24l01_get_data(middleware_nrf24l01_car_data_t * car_data);

void task_control_nrf24l01_control(middleware_nrf24l01_joystick_data_t *joystick_data);
#endif
