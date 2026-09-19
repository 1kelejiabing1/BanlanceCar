/**
 * @file    middleware_pid.c
 * @brief   位置式 PID 中间件
 *
 * 位置式 PID 公式：Out = Kp*Error + Ki*ErrorInt - Kd*(Actual - ActualLast)
 * 说明：
 *   - 积分项带限幅，微分项采用"微分先行"（对实际值微分而非误差微分），
 *     可减小目标值突变时造成的微分冲击
 *   - 输出带偏移补偿（克服死区）和限幅
 */
#include "middleware_pid.h"

/**
 * @brief   PID 参数初始化：清零所有状态量
 * @param   p  PID 对象指针
 * @retval  None
 */
void middleware_pid_init(middleware_pid_t *p)
{
	/*把PID表示状态的参数清零，避免之前遗留的参数对本次启动造成影响*/
	p->Target = 0;
	p->Actual = 0;
	p->ActualLast = 0;
	p->Out = 0;
	p->Error = 0;
	p->Error_Pre = 0;
	p->ErrorInt = 0;
}

/**
 * @brief   执行一次 PID 计算，更新输出
 * @note    调用前需先设置 Target（目标值）和 Actual（实际值）
 * @param   p  PID 对象指针
 * @retval  None
 */
void middleware_pid_update(middleware_pid_t *p)
{
	/*获取本次误差和上次误差*/
	p->Error_Pre = p->Error;					//获取上次误差
	p->Error = p->Target - p->Actual;		//获取本次误差，目标值减实际值，即为误差值
	
	/*外环误差积分（累加）*/
	if (p->Ki != 0)				//如果Ki不为0
	{
		p->ErrorInt += p->Error;	//进行误差积分
		
		/*误差积分限幅*/
		if (p->ErrorInt > p->ErrorIntMax) {p->ErrorInt = p->ErrorIntMax;}	//限制误差积分最大为结构体指定的ErrorIntMax
		if (p->ErrorInt < p->ErrorIntMin) {p->ErrorInt = p->ErrorIntMin;}	//限制误差积分最小为结构体指定的ErrorIntMin
	}
	else							//否则
	{
		p->ErrorInt = 0;			//误差积分直接归0
	}
	
	/*PID计算：位置式 PID，微分项采用"微分先行"（对实际值微分，减小目标突变冲击）*/
	p->Out = p->Kp * p->Error
		   + p->Ki * p->ErrorInt
		   - p->Kd * (p->Actual - p->ActualLast);
	
	/*输出偏移*/
	if (p->Out > 0) {p->Out += p->OutOffset;}		//如果输出值为正，则加上结构体指定的OutOffset
	if (p->Out < 0) {p->Out -= p->OutOffset;}		//如果输出值为负，则减去结构体指定的OutOffset
	
	/*输出限幅*/
	if (p->Out > p->OutMax) {p->Out = p->OutMax;}	//限制输出值最大为结构体指定的OutMax
	if (p->Out < p->OutMin) {p->Out = p->OutMin;}	//限制输出值最小为结构体指定的OutMin
	
	/*获取上次实际值*/
	p->ActualLast = p->Actual;		//本轮计算后进行变量传递，下轮计算时Actual1即为上次实际值
}
