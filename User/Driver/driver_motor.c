/**
 * @file    driver_motor.c
 * @brief   电机驱动：PWM 调速（TIM2）+ 方向 GPIO（TB6612FNG）
 *
 * 速度参数范围 -100 ~ +100：
 *   - 符号决定旋转方向（通过 AIN1/AIN2 或 BIN1/BIN2 控制）
 *   - 绝对值决定 PWM 占空比（写入 TIM2 比较寄存器）
 */
#include "driver_motor.h"
#include <stdlib.h>
#include "tim.h"

/**
 * @brief   电机初始化（占位，PWM 定时器已由 CubeMX 配置）
 * @retval  None
 */
void driver_motor_init(void)
{
}

/**
 * @brief   开启电机（启动 PWM 输出）
 * @retval  None
 */
void driver_motor_start(void)
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
}

/**
 * @brief   关闭电机（停止 PWM 输出）
 * @retval  None
 */
void driver_motor_stop(void)
{
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
}

/**
 * @brief   设置左电机速度
 * @param   speed  速度值，范围 -100 ~ +100（超出自动限幅）
 * @retval  None
 */
void driver_motor_set_left_speed(int8_t speed)
{
    if(abs(speed) > 100)
    {
        speed = (speed > 100)?100:-100;
    }
    if(speed >= 0)
    {
        HAL_GPIO_WritePin(LEFT_MOTOR_AIN_1_GPIO_Port, LEFT_MOTOR_AIN_1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LEFT_MOTOR_AIN_2_GPIO_Port, LEFT_MOTOR_AIN_2_Pin, GPIO_PIN_RESET);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, speed);
    }
    else
    {
        HAL_GPIO_WritePin(LEFT_MOTOR_AIN_1_GPIO_Port, LEFT_MOTOR_AIN_1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LEFT_MOTOR_AIN_2_GPIO_Port, LEFT_MOTOR_AIN_2_Pin, GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, -speed);
    }
}

/**
 * @brief   设置右电机速度
 * @param   speed  速度值，范围 -100 ~ +100（超出自动限幅）
 * @retval  None
 */
void driver_motor_set_right_speed(int8_t speed)
{
    if(abs(speed) > 100)
    {
        speed = (speed > 100)?100:-100;
    }
    if(speed >= 0)
    {
        HAL_GPIO_WritePin(RIGHT_MOTOR_BIN_1_GPIO_Port, RIGHT_MOTOR_BIN_1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RIGHT_MOTOR_BIN_2_GPIO_Port, RIGHT_MOTOR_BIN_2_Pin, GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, speed);
    }
    else
    {
        HAL_GPIO_WritePin(RIGHT_MOTOR_BIN_1_GPIO_Port, RIGHT_MOTOR_BIN_1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(RIGHT_MOTOR_BIN_2_GPIO_Port, RIGHT_MOTOR_BIN_2_Pin, GPIO_PIN_RESET);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, -speed);
    }
}


