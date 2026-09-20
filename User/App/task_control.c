/**
 * @file    task_control.c
 * @brief   平衡控制任务：姿态解算 + 三环串级 PID
 *
 * 核心控制逻辑：
 *   1. 读取 MPU6500，通过互补滤波解算俯仰角
 *   2. 直立环（角度环）15ms 周期，维持车体直立
 *   3. 速度环 + 转向环 60ms 周期，速度环输出作为直立环目标角度，
 *      转向环输出作为左右轮差速
 *   4. 安全保护：车体倾斜超过 ±50° 自动停止电机
 *
 * 控制量关系：
 *   平均速度 = 速度环输出（作为角度环目标）
 *   左轮速度 = 平均速度 + 差速/2
 *   右轮速度 = 平均速度 - 差速/2
 */
#include "task_control.h"
#include <math.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "driver_mpu6500.h"
#include "middleware_pid.h"
#include "driver_motor.h"
#include "driver_oled.h"
#include "app_debug.h"
#include "driver_encoder.h"


/* 控制周期相关常量 */
#define PERIOD_MS                   (15)      /* 直立环控制周期(ms) */
#define PERIOD_S                    (0.015)   /* 直立环控制周期(s) */
#define ALPHA                       (0.01)    /* 互补滤波系数（加速度计权重） */
#define GYRO_Y_ZERO_PITCH_ERROR     (0.8)     /* 陀螺仪Y轴零飘补偿 */
#define PITCH_ZERO_PITCH_ERROR      (0.5)     /* 俯仰角零飘补偿 */


/* 直立环（角度环）PID 参数 */
static middleware_pid_t s_task_control_angle_pid = {
    .Kp = 4.0,
	.Ki = 0.15,
	.Kd = 5.0,

    .ErrorIntMax = 400,
	.ErrorIntMin = -400,

	.OutMax = 100,
	.OutMin = -100,

	.OutOffset = 4,
};

/* 速度环 PID 参数 */
static middleware_pid_t s_task_control_speed_pid = {
    .Kp = 1.1,
	.Ki = 0.05,
	.Kd = 0,

    .ErrorIntMax = 100,
	.ErrorIntMin = -100,

	.OutMax = 20,
	.OutMin = -20,

	.OutOffset = 0,
};

/* 转向环 PID 参数 */
static middleware_pid_t s_task_control_turn_pid = {
    .Kp = 4.0,
	.Ki = 3.0,
	.Kd = 0,

    .ErrorIntMax = 20,
	.ErrorIntMin = -20,

	.OutMax = 50,
	.OutMin = -50,

	.OutOffset = 0,
};

static float s_task_control_angle_acc;			    /* 由加速度计得到的角度值 */
static float s_task_control_angle_gyro;		        /* 由陀螺仪得到的角度值（积分后） */
static float s_task_control_angle;			        /* 互补滤波后的角度值，准确且无漂移 */

static int16_t s_task_control_ave_speed;            /* 左右轮平均目标速度 */
static int8_t s_task_control_left_speed;            /* 左轮目标速度 */
static int8_t s_task_control_right_speed;           /* 右轮目标速度 */

static float s_task_control_diff_speed = 0;         /* 转向环输出的左右轮差速 */

volatile uint8_t g_task_control_run_flag = 0;       /* 平衡运行标志：1=运行，0=停止 */

uint8_t g_task_control_time_count = 0;              /* 速度环分频计数（4 分频） */
static float s_task_control_left_actual_speed;      /* 左轮实际速度 */
static float s_task_control_right_actual_speed;     /* 右轮实际速度 */

static void task_control_get_speed(void);

/**
 * @brief   平衡控制任务主体
 * @param   pvParameters  未使用
 * @retval  None
 */
void task_control(void *pvParameters)
{
    middleware_pid_init(&s_task_control_angle_pid);
    driver_motor_start();
    driver_encoder_start();
    mpu6500_raw_data_t raw;
    mpu6500_scale_data_t scaled;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1) {
        if (driver_mpu6500_read_raw(&raw))
        {
            /* 1. 由加速度计解算俯仰角（atan2 求角度，扣除零飘） */
            s_task_control_angle_acc = -atan2(raw.accel_x, raw.accel_z) / 3.14159 * 180;
            s_task_control_angle_acc -= PITCH_ZERO_PITCH_ERROR;

            /* 2. 由陀螺仪积分角度（扣除零飘） */
            driver_mpu6500_convert(&raw, &scaled);
            scaled.gyro_y -= GYRO_Y_ZERO_PITCH_ERROR;
            s_task_control_angle_gyro = s_task_control_angle + scaled.gyro_y * PERIOD_S;

            /* 3. 一阶互补滤波融合两者，得到最终角度 */
            s_task_control_angle = ALPHA * s_task_control_angle_acc + (1 - ALPHA) * s_task_control_angle_gyro;

            /* 安全保护：车体倾斜超过 ±50° 自动停止 */
            if(abs(s_task_control_angle) > 50)
            {
                /* uint8_t 写操作在 32 位 MCU 上为原子操作，无需加锁 */
                g_task_control_run_flag = 0;
            }

            if(g_task_control_run_flag == 1)
            {
                /* 直立环：角度环输出作为平均速度 */
                s_task_control_angle_pid.Actual = s_task_control_angle;
                middleware_pid_update(&s_task_control_angle_pid);
                s_task_control_ave_speed = -s_task_control_angle_pid.Out;

                /* 叠加转向差速，得到左右轮目标速度 */
                s_task_control_left_speed = s_task_control_ave_speed + s_task_control_diff_speed / 2;
                s_task_control_right_speed = s_task_control_ave_speed - s_task_control_diff_speed / 2;
                driver_motor_set_left_speed(s_task_control_left_speed);
                driver_motor_set_right_speed(s_task_control_right_speed);
            }
            else
            {
                driver_motor_set_left_speed(0);
                driver_motor_set_right_speed(0);
                /* 停止时清空 PID 状态，避免对下次启动产生影响 */
                middleware_pid_init(&s_task_control_angle_pid);
            }
        }

        /* 速度环和转向环每 15 * 4 = 60ms 更新一次 */
        if(g_task_control_time_count++ >= 4)
        {
            g_task_control_time_count = 0;
            task_control_get_speed();

            /* 速度环 -> 角度环：速度环输出作为角度环目标 */
            s_task_control_speed_pid.Actual = (s_task_control_left_actual_speed + s_task_control_right_actual_speed) / 2.0;
            middleware_pid_update(&s_task_control_speed_pid);
            s_task_control_angle_pid.Target = s_task_control_speed_pid.Out;

            /* 转向环：左右轮速度差控制方向 */
            s_task_control_turn_pid.Actual = s_task_control_left_actual_speed - s_task_control_right_actual_speed;
            middleware_pid_update(&s_task_control_turn_pid);
            s_task_control_diff_speed = s_task_control_turn_pid.Out;
        }
        UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
        debug_printf("%d\r\n", stackLeft);
        vTaskDelayUntil(&xLastWakeTime, PERIOD_MS);
    }
}

/**
 * @brief   在 OLED 上显示小车状态（目标速度 / 角度 / 实际速度）
 * @note    仅写入 OLED 显存并刷新，需在 OLED 任务中周期调用
 * @retval  None
 */
void task_control_oled_show(void)
{
    int16_t angle_scaled   = (int16_t)(s_task_control_angle * 100);
    int16_t left_scaled    = (int16_t)(s_task_control_left_actual_speed * 100);
    int16_t right_scaled   = (int16_t)(s_task_control_right_actual_speed * 100);
    int16_t angle_frac     = angle_scaled % 100;  if (angle_frac < 0) angle_frac = -angle_frac;
    int16_t left_frac      = left_scaled  % 100;  if (left_frac  < 0) left_frac  = -left_frac;
    int16_t right_frac     = right_scaled % 100;  if (right_frac < 0) right_frac = -right_frac;

    /* 第 1 行：左右轮目标速度 */
    OLED_ShowString(1, 1, "LS:");
    OLED_ShowSignedNum(1, 4, s_task_control_left_speed, 3);
    OLED_ShowString(1, 9, "RS:");
    OLED_ShowSignedNum(1, 12, s_task_control_right_speed, 3);

    /* 第 2 行：角度 */
    OLED_ShowString(2, 1, "Angle:");
    OLED_ShowSignedNum(2, 7, angle_scaled / 100, 3);
    OLED_ShowString(2, 11, ".");
    OLED_ShowNum(2, 12, angle_frac, 2);

    /* 第 3 行：左轮实际速度 */
    OLED_ShowString(3, 1, "LAS:");
    OLED_ShowSignedNum(3, 5, left_scaled / 100, 3);
    OLED_ShowString(3, 9, ".");
    OLED_ShowNum(3, 10, left_frac, 2);

    /* 第 4 行：右轮实际速度 */
    OLED_ShowString(4, 1, "RAS:");
    OLED_ShowSignedNum(4, 5, right_scaled / 100, 3);
    OLED_ShowString(4, 9, ".");
    OLED_ShowNum(4, 10, right_frac, 2);
	OLED_Refresh();
}

/**
 * @brief   设置 PID 参数（供蓝牙调参使用）
 * @param   pid        选择目标环：角度环 / 速度环 / 转向环
 * @param   param_mode 选择参数：P / I / D / Offset / Target
 * @param   value      参数值
 * @retval  None
 */
void task_control_set_pid_param(task_control_pid_t pid, task_control_pid_param_t param_mode, float value)
{
    switch(pid)
    {
        case TASK_CONTROL_ANGLE_PID:
        {
            switch(param_mode)
            {
                case TASK_CONTROL_PID_P:
                    s_task_control_angle_pid.Kp = value;
                    break;
                case TASK_CONTROL_PID_I:
                    s_task_control_angle_pid.Ki = value;
                    break;
                case TASK_CONTROL_PID_D:
                    s_task_control_angle_pid.Kd = value;
                    break;
                case TASK_CONTROL_PID_OFFSET:
                    s_task_control_angle_pid.OutOffset = value;
                    break;
            }
            break;
        }

        case TASK_CONTROL_SPEED_PID:
        {
            switch(param_mode)
            {
                case TASK_CONTROL_PID_P:
                    s_task_control_speed_pid.Kp = value;
                    break;
                case TASK_CONTROL_PID_I:
                    s_task_control_speed_pid.Ki = value;
                    break;
                case TASK_CONTROL_PID_D:
                    s_task_control_speed_pid.Kd = value;
                    break;
                case TASK_CONTROL_PID_OFFSET:
                    s_task_control_speed_pid.OutOffset = value;
                    break;
                case TASK_CONTROL_PID_TARGET:
                    s_task_control_speed_pid.Target = value;
                    break;
            }
            break;
        }
        case TASK_CONTROL_TURN_PID:
        {
            switch(param_mode)
            {
                case TASK_CONTROL_PID_P:
                    s_task_control_turn_pid.Kp = value;
                    break;
                case TASK_CONTROL_PID_I:
                    s_task_control_turn_pid.Ki = value;
                    break;
                case TASK_CONTROL_PID_D:
                    s_task_control_turn_pid.Kd = value;
                    break;
                case TASK_CONTROL_PID_OFFSET:
                    s_task_control_turn_pid.OutOffset = value;
                    break;
                case TASK_CONTROL_PID_TARGET:
                    s_task_control_turn_pid.Target = value;
                    break;
            }
            break;
        }
    }

    debug_printf("Kp:%d.%d\r\n",PRINT_F2(s_task_control_angle_pid.Kp));
    debug_printf("Kp:%f\tKi:%f\tKd:%f\r\n",s_task_control_angle_pid.Kp,s_task_control_angle_pid.Ki,s_task_control_angle_pid.Kd);
}

/**
 * @brief   根据 NRF24L01 摇杆数据设置小车目标速度与转向（供无线遥控使用）
 * @param   joystick_data  遥控摇杆数据（-100 ~ +100 缩放为 -4 ~ +4）
 * @retval  None
 */
void task_control_nrf24l01_control(middleware_nrf24l01_joystick_data_t *joystick_data)
{
    s_task_control_speed_pid.Target = joystick_data->left_vertical / 25.0;
    s_task_control_turn_pid.Target = joystick_data->right_horizontal / 25.0;
}

/**
 * @brief   获取小车状态数据（供 NRF24L01 回传使用）
 * @param   car_data  输出：目标速度 / 角度 / 实际速度
 * @retval  None
 */
void task_control_nrf24l01_get_data(middleware_nrf24l01_car_data_t * car_data)
{
    car_data->left_speed = s_task_control_left_speed;
    car_data->right_speed = s_task_control_right_speed;
    car_data->angle = s_task_control_angle;
    car_data->left_actual_speed = s_task_control_left_actual_speed;
    car_data->right_actual_speed = s_task_control_right_actual_speed;
}

/**
 * @brief   读取并换算左右轮实际转速
 * @note    编码器磁铁每转一圈计次增量为 44，读取间隔为 60ms（0.06s）
 *          磁铁转速(转/s) = 计次增量 / 44 / 0.06
 *          电机带减速箱（减速比 9.27666），输出轴转速 = 磁铁转速 / 9.27666
 *          右编码器正方向与实际运动方向相反，故取负数修正
 * @retval  None
 */
static void task_control_get_speed(void)
{
    s_task_control_left_actual_speed = driver_encoder_get_left_count() / 44 / 0.06 / 9.27666;
    s_task_control_right_actual_speed =  -driver_encoder_get_right_count() / 44 / 0.06 / 9.27666;
}
