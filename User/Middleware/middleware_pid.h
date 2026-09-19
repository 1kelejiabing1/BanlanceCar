/**
 * @file    middleware_pid.h
 * @brief   位置式 PID 中间件接口声明
 */
#ifndef MIDDLEWARE_PID_H
#define MIDDLEWARE_PID_H

#include "main.h"
#include <stdint.h>

/**
 * @brief   PID 对象：保存目标值、状态量、参数与限幅配置
 */
typedef struct {
    float Target;		//目标值，由用户设定
	float Actual;		//实际值，从传感器读取
	float ActualLast;	//上次实际值
	float Out;			//输出值，作用于执行器

	float Kp;			//比例项权重
	float Ki;			//积分项权重
	float Kd;			//微分项权重
	
	float Error;		//本次误差
	float Error_Pre;	//上次误差
	float ErrorInt;		//误差积分
	
	float ErrorIntMax;	//误差积分的最大值
	float ErrorIntMin;	//误差积分的最小值
	
	float OutMax;		//输出限幅的最大值
	float OutMin;		//输出限幅的最小值
	
	float OutOffset;	//输出偏移
} middleware_pid_t;

void middleware_pid_init(middleware_pid_t *p);

void middleware_pid_update(middleware_pid_t *p);
#endif
