#include "driver_mpu6500.h"
#include "app_debug.h"
#include "i2c.h"

/*============================================================================
 * 静态变量
 *============================================================================*/

static I2C_HandleTypeDef *g_hi2c = &hi2c2;

/*============================================================================
 * 内部辅助
 *============================================================================*/

/**
 * @brief   写单个寄存器
 */
static bool driver_mpu6500_write_reg(uint8_t reg, uint8_t value)
{
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Write(g_hi2c, MPU6500_ADDR << 1, reg, I2C_MEMADD_SIZE_8BIT, &value, 1, HAL_MAX_DELAY);
    return ret == HAL_OK;
}

/**
 * @brief   读多个寄存器
 */
static bool driver_mpu6500_read_regs(uint8_t reg, uint8_t *data, uint8_t len)
{
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(g_hi2c, MPU6500_ADDR << 1, reg, I2C_MEMADD_SIZE_8BIT, data, len, HAL_MAX_DELAY);
    return ret == HAL_OK;
}

/*============================================================================
 * API 函数
 *============================================================================*/

/**
 * @brief   mpu6500 初始化
 * @param   hi2c   I2C2 句柄
 * @retval  true   初始化成功
 *
 * 步骤：
 *   1. 读取 WHO_AM_I 确认器件存在
 *   2. 唤醒器件（退出睡眠模式）
 *   3. 配置采样率和滤波器（可选，这里保持默认）
 */
bool driver_mpu6500_init(void)
{
    uint8_t whoami = 0;

    /* ---- 1. 读取 WHO_AM_I ---- */
    if (!driver_mpu6500_read_regs(MPU6500_REG_WHO_AM_I, &whoami, 1))
    {
        debug_printf("[mpu6500] I2C 通信失败\r\n");
        return false;
    }

    if (whoami != 0x70)
    {
        debug_printf("[mpu6500] WHO_AM_I 错误: 0x%02X (期望 0x70)\r\n", whoami);
        return false;
    }

    debug_printf("[mpu6500] WHO_AM_I = 0x%02X, 器件正常\r\n", whoami);

    /* ---- 2. 唤醒器件（清零 SLEEP 位）---- */
    if (!driver_mpu6500_write_reg(MPU6500_REG_PWR_MGMT1, 0x00))
    {
        debug_printf("[mpu6500] 唤醒失败\r\n");
        return false;
    }
    /* ---- 3. 电源管理寄存器2，保持默认值0，所有轴均不待机 ---- */
    driver_mpu6500_write_reg(MPU6500_REG_PWR_MGMT2, 0x00);		//
    
    /* ---- 4. 采样率 = 1kHz / (1 + 7) = 125Hz */
    driver_mpu6500_write_reg(MPU6500_REG_SMPLRT_DIV, 0x07);

    /* ---- 5. 数字低通滤波器 256Hz (DLPF_CFG = 0) ---- */
    driver_mpu6500_write_reg(MPU6500_REG_CONFIG, 0x00);
    
    /* ---- 3. 配置加速度计量程 ±2g ---- */
    driver_mpu6500_write_reg(MPU6500_REG_ACCEL_CONFIG, 0x18);

    /* ---- 4. 配置陀螺仪量程 ±250°/s ---- */
    driver_mpu6500_write_reg(MPU6500_REG_GYRO_CONFIG, 0x18);

    HAL_Delay(50);

    debug_printf("[MPU6500] 初始化完成\r\n");

    return true;
}

/**
 * @brief   读取原始传感器数据
 * @param   raw   输出：原始 ADC 值
 * @retval  true  读取成功
 *
 * 从 ACCEL_XOUT_H (0x3B) 开始连续读取 14 字节：
 *   [0:1]  Accel X
 *   [2:3]  Accel Y
 *   [4:5]  Accel Z
 *   [6:7]  Temperature
 *   [8:9]  Gyro X
 *   [10:11] Gyro Y
 *   [12:13] Gyro Z
 */
bool driver_mpu6500_read_raw(mpu6500_raw_data_t *raw)
{
    uint8_t buf[14];

    if (!driver_mpu6500_read_regs(MPU6500_REG_ACCEL_XOUT_H, buf, 14))
    {
        return false;
    }

    raw->accel_x = (int16_t)((buf[0] << 8) | buf[1]);
    raw->accel_y = (int16_t)((buf[2] << 8) | buf[3]);
    raw->accel_z = (int16_t)((buf[4] << 8) | buf[5]);
    raw->temp = (int16_t)((buf[6] << 8) | buf[7]);
    raw->gyro_x = (int16_t)((buf[8] << 8) | buf[9]);
    raw->gyro_y = (int16_t)((buf[10] << 8) | buf[11]);
    raw->gyro_z = (int16_t)((buf[12] << 8) | buf[13]);

    return true;
}

/**
 * @brief   将原始 ADC 值换算为物理量
 * @param   raw     原始数据
 * @param   scaled  输出：物理量
 */
void driver_mpu6500_convert(const mpu6500_raw_data_t *raw, mpu6500_scale_data_t *scaled)
{
    /* 当前项目只需俯仰角速度 gyro_y，其余轴换算因子见 driver_mpu6500.h，
       如需使用请参考头文件中的量程换算关系自行解开对应行 */
    scaled->gyro_y = raw->gyro_y / GYRO_SCALE;
}

/**
 * @brief   自检：读取 WHO_AM_I 验证通信
 * @retval  true  器件存在且通信正常
 */
bool driver_mpu6500_test(void)
{
    uint8_t whoami = 0;
    if (driver_mpu6500_read_regs(MPU6500_REG_WHO_AM_I, &whoami, 1))
    {
        return (whoami == 0x68);
    }
    return false;
}

/**
 * @brief   一次性读取 + 换算
 * @param   scaled  输出：物理量数据
 * @retval  true    读取成功
 */
bool driver_mpu6500_get_scaled(mpu6500_scale_data_t *scaled)
{
    mpu6500_raw_data_t raw;
    if (driver_mpu6500_read_raw(&raw))
    {
        driver_mpu6500_convert(&raw, scaled);
        return true;
    }
    return false;
}
