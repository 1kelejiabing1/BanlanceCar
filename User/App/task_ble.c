/**
 * @file    task_ble.c
 * @brief   蓝牙指令处理任务：解析并执行蓝牙接收的调参 / 遥控指令
 *
 * 从 middleware_ble 的帧队列中取出完整帧，按帧内容分类处理：
 *   1. "slider"   帧 —— 通过滑杆修改 PID 参数
 *   2. "joystick" 帧 —— 通过摇杆遥控小车移动
 */
#include "task_ble.h"
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "driver_ble.h"
#include "middleware_ble.h"
#include "app_debug.h"
#include "task_control.h"

/**
 * @brief   蓝牙指令处理任务主体
 * @param   pvParameters  未使用
 * @retval  None
 */
void task_ble(void *pvParameters)
{
    while (1)
    {
        ble_protocol_frame_t ble_protocol_frame;
		
        if (xQueueReceive(middleware_bleframe_queue, &ble_protocol_frame, portMAX_DELAY) == pdTRUE)
        {
			char temp_buf[128];
			memcpy(temp_buf,  ble_protocol_frame.ble_protocol_frame, ble_protocol_frame.length);
			temp_buf[ble_protocol_frame.length] = '\0';
            char *strprt = NULL;
            char *tag = strtok_r(temp_buf, ",", &strprt);
            // 接收到滑杆数据(用于调参)
            if(strcmp(tag, "slider") == 0)
            {
                // 接收到滑杆数据，本项目通过滑杆修改PID参数
                char *parameter_name = strtok_r(NULL, ",", &strprt);
                char *value = strtok_r(NULL, ",", &strprt);
                task_control_pid_t pid;
                task_control_pid_param_t pid_param;
                // 调整角度环参数
                if(strcmp(parameter_name, "AngleKp") == 0)
                {
                    pid = TASK_CONTROL_ANGLE_PID;
                    pid_param = TASK_CONTROL_PID_P;
                }
                else if(strcmp(parameter_name, "AngleKi") == 0)
                {
                    pid = TASK_CONTROL_ANGLE_PID;
                    pid_param = TASK_CONTROL_PID_I;
                }
                else if(strcmp(parameter_name, "AngleKd") == 0)
                {
                    pid = TASK_CONTROL_ANGLE_PID;
                    pid_param = TASK_CONTROL_PID_D;
                }
                else if(strcmp(parameter_name, "AngleOffset") == 0)
                {
                    pid = TASK_CONTROL_ANGLE_PID;
                    pid_param = TASK_CONTROL_PID_OFFSET;
                }
                else if(strcmp(parameter_name, "SpeedKp") == 0)
                {
                    pid = TASK_CONTROL_SPEED_PID;
                    pid_param = TASK_CONTROL_PID_P;
                }
                else if(strcmp(parameter_name, "SpeedKi") == 0)
                {
                    pid = TASK_CONTROL_SPEED_PID;
                    pid_param = TASK_CONTROL_PID_I;
                }
                else if(strcmp(parameter_name, "SpeedKd") == 0)
                {
                    pid = TASK_CONTROL_SPEED_PID;
                    pid_param = TASK_CONTROL_PID_D;
                }
                else if(strcmp(parameter_name, "SpeedOffset") == 0)
                {
                    pid = TASK_CONTROL_SPEED_PID;
                    pid_param = TASK_CONTROL_PID_OFFSET;
                }
                else if(strcmp(parameter_name, "TurnKp") == 0)
                {
                    pid = TASK_CONTROL_TURN_PID;
                    pid_param = TASK_CONTROL_PID_P;
                }
                else if(strcmp(parameter_name, "TurnKi") == 0)
                {
                    pid = TASK_CONTROL_TURN_PID;
                    pid_param = TASK_CONTROL_PID_I;
                }
                else if(strcmp(parameter_name, "TurnKd") == 0)
                {
                    pid = TASK_CONTROL_TURN_PID;
                    pid_param = TASK_CONTROL_PID_D;
                }
                else if(strcmp(parameter_name, "TurnOffset") == 0)
                {
                    pid = TASK_CONTROL_TURN_PID;
                    pid_param = TASK_CONTROL_PID_OFFSET;
                }
                task_control_set_pid_param(pid, pid_param, strtof(value, NULL));
            }
            // 接收到摇杆数据(用于遥控小车移动)
            if(strcmp(tag, "joystick") == 0)
            {
                // 左摇杆前后值
                float left_horizontal = (float)strtol(strtok_r(NULL, ",", &strprt), NULL, 10);
                float left_vertical = (float)strtol(strtok_r(NULL, ",", &strprt), NULL, 10);
                float right_horizontal = (float)strtol(strtok_r(NULL, ",", &strprt), NULL, 10);
                float right_vertical = (float)strtol(strtok_r(NULL, ",", &strprt), NULL, 10);
                // 缩放
                left_vertical /= 25;
                right_horizontal /= 25;
                task_control_set_pid_param(TASK_CONTROL_SPEED_PID, TASK_CONTROL_PID_TARGET, left_vertical);
                task_control_set_pid_param(TASK_CONTROL_TURN_PID, TASK_CONTROL_PID_TARGET, right_horizontal);
            }
            
        }
    }
}
