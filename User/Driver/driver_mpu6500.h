/**
 * @file    MPU6500.h
 * @brief   MPU6500 六轴传感器驱动 — I2C 通信
 *
 * 硬件连接：
 *   SCL → PB10 (I2C2)
 *   SDA → PB11 (I2C2)
 *   AD0 → GND (器件地址 0x68)
 *
 * 数据单位：
 *   加速度计：±2g 量程  → 16384 LSB/g
 *   陀螺仪：  ±250°/s  → 131   LSB/(°/s)
 *   温度：    原始值需换算  → °C = raw / 340 + 36.53
 */

#ifndef __MPU6500_H
#define __MPU6500_H

#include "main.h"
#include <stdbool.h>
#include <stdint.h>

/*============================================================================
 * 宏定义
 *============================================================================*/

/* I2C 地址 (AD0=GND) */
#define MPU6500_ADDR 0x68

/* 寄存器地址 */
#define MPU6500_REG_WHO_AM_I 0x75     /* 复位值: 0x68 */
#define MPU6500_REG_PWR_MGMT1 0x6B    /* 电源管理1 */
#define MPU6500_REG_PWR_MGMT2 0x6C    /* 电源管理2 */
#define MPU6500_REG_SMPLRT_DIV 0x19   /* 采样率分频 */
#define MPU6500_REG_CONFIG 0x1A       /* 低通滤波器配置 */
#define MPU6500_REG_GYRO_CONFIG 0x1B  /* 陀螺仪量程 */
#define MPU6500_REG_ACCEL_CONFIG 0x1C /* 加速度计量程 */
#define MPU6500_REG_ACCEL_XOUT_H 0x3B /* 加速度计 X 高字节 → 共 14 字节连续读取 */

/* 换算因子 */
/*
    加速度计量程与灵敏度对应关系：
    AFS_SEL     Full Scale Range    LSB Sensitivity     写入 0x1C 的具体值
    0           ±2g                 16384 LSB/g         0x00
    1           ±4g                 8192 LSB/g          0x08
    2           ±8g                 4096 LSB/g          0x10
    3           ±16g                2048 LSB/g          0x18
*/
/*
    陀螺仪量程与灵敏度对应关系：
    FS_SEL     Full Scale Range    LSB Sensitivity      写入 0x1B 的具体值
    0           ±250°/s             131   LSB/(°/s)     0x00
    1           ±500°/s             65.5  LSB/(°/s)     0x08
    2           ±1000°/s            32.8  LSB/(°/s)     0x10
    3           ±2000°/s            16.384LSB/(°/s)     0x18
*/
#define ACCEL_SCALE (2048.0f) /* ±2g: 16384 LSB/g */
#define GYRO_SCALE (16.384f)    /* ±250°/s: 131 LSB/(°/s) */
#define TEMP_OFFSET (36.53f)   /* 温度偏移 */
#define TEMP_SCALE (340.0f)    /* 温度斜率: 340 LSB/°C */

/*============================================================================
 * 数据结构
 *============================================================================*/

/**
 * @brief   原始传感器数据（ADC 值，未换算）
 */
typedef struct
{
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t temp;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
}mpu6500_raw_data_t;

/**
 * @brief   换算后的物理量数据
 */
typedef struct
{
    float accel_x; /* g */
    float accel_y; /* g */
    float accel_z; /* g */
    float temp;    /* °C */
    float gyro_x;  /* °/s */
    float gyro_y;  /* °/s */
    float gyro_z;  /* °/s */
}mpu6500_scale_data_t;

/*============================================================================
 * 函数声明
 *============================================================================*/

bool driver_mpu6500_init(void);
bool driver_mpu6500_read_raw(mpu6500_raw_data_t *raw);
void driver_mpu6500_convert(const mpu6500_raw_data_t *raw, mpu6500_scale_data_t *scaled);
bool driver_mpu6500_test(void);
bool driver_mpu6500_get_scaled(mpu6500_scale_data_t *scaled);

#endif /* __MPU6500_H */
