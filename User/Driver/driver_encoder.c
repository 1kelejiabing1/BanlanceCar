/**
 * @file    driver_encoder.c
 * @brief   霍尔编码器驱动：基于定时器编码器模式（TIM3 / TIM4）
 *
 * 定时器工作在编码器接口模式，自动对电机霍尔信号计数。
 * 读取计数后立即清零，返回的是"自上次读取以来的增量"。
 */
#include "driver_encoder.h"
#include "tim.h"

/**
 * @brief   编码器初始化（占位，定时器外设已由 CubeMX 配置）
 * @retval  None
 */
void driver_encoder_init(void)
{
}
/**
 * @brief   启动左右编码器计数并清零
 * @retval  None
 */
void driver_encoder_start(void)
{
    // 开启左右编码器
    HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
    HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL);
    // 清零编码器数
    __HAL_TIM_SET_COUNTER(&htim3, 0);
    __HAL_TIM_SET_COUNTER(&htim4, 0);
}

/**
 * @brief   左编码器计数清零
 * @retval  None
 */
void driver_encoder_clear_left_count(void)
{
    __HAL_TIM_SET_COUNTER(&htim3, 0);
}

/**
 * @brief   右编码器计数清零
 * @retval  None
 */
void driver_encoder_clear_right_count(void)
{
    __HAL_TIM_SET_COUNTER(&htim4, 0);
}

/**
 * @brief   获取左编码器计数增量并清零
 * @retval  本次读取周期内的计数值
 */
int16_t driver_encoder_get_left_count(void)
{
    int16_t count;
    count = __HAL_TIM_GET_COUNTER(&htim3);
    driver_encoder_clear_left_count();
    return count;
}

/**
 * @brief   获取右编码器计数增量并清零
 * @retval  本次读取周期内的计数值
 */
int16_t driver_encoder_get_right_count(void)
{
    int16_t count;
    count = __HAL_TIM_GET_COUNTER(&htim4);
    driver_encoder_clear_right_count();
    return count;
}


