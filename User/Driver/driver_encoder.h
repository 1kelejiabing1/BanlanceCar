/**
 * @file    driver_encoder.h
 * @brief   霍尔编码器驱动接口声明
 */
#ifndef DRIVER_ENCODER_H
#define DRIVER_ENCODER_H

#include "main.h"
#include <stdint.h>

void driver_encoder_init(void);

void driver_encoder_start(void);

/* 获取左右编码器计数增量（读取后自动清零） */
int16_t driver_encoder_get_left_count(void);
int16_t driver_encoder_get_right_count(void);

/* 左右编码器计数清零 */
void driver_encoder_clear_left_count(void);
void driver_encoder_clear_right_count(void);

#endif
