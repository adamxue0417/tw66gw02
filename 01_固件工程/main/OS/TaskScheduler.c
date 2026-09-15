#include "TaskScheduler.h"
/**
 * @file    TaskScheduler.c
 * @brief   系统任务调度文件
 * @author  niu
 * @date    2026.02.24
 * @version 1.0
 * 
 * 详细描述:该文件用于系统的任务调度。
   通过将SCH_Update函数放入定时中断1ms的时基中，SCH_Dispatch_Tasks函数放入主函数的while(1)循环中，
	 利用SCH_Add_Task将任务函数加入到线程中，便可以实现任务的定时调度功能。
 */
/***************************************************************************************************************************************************************/
sTask SCH_tasks_G[SCH_MAX_TASKS];  //任务队列
SCH_Error_TypeDef Error_Code_G = NOT_ERROR;
static uint16_t Error_tick_count_G;//错误清除倒计数：每次主循环调度递减，不是毫秒
static uint8_t Last_error_code_G = NOT_ERROR;//上次的错误代码；清除速度取决于主循环执行次数
/**
 * @brief   SCH_Init函数功能简述
 * @param   none    
 * @return  none
 * @note    none
 * 
 * 详细说明：此函数为任务调度初始化函数，通过让调度机恢复到初始化的进程来，为接下来的任务调度做准备。
 */
void SCH_Init(void)
{
    uint16_t i;
    for(i = 0; i < SCH_MAX_TASKS; i++)
    {
        SCH_Delete_Task(i);
    }
    Error_Code_G = NOT_ERROR;
    //systick定时器初始化
}
/**
 * @brief   SCH_Update函数功能简述
 * @param   none    
 * @return  none
 * @note    none
 * 
 * 详细说明：此函数为任务调度更新函数，需要放入到定时器中断中，作为时基触发，定时刷新此函数。
   所有的任务调度，都是在这个函数的基础上刷新调度，这个函数的调度时间是所有函数调度的最小时基单元。
   此函数通常设定在1ms的定时器中断中进行调度。
 */
/* 中断只累加 RunMe，业务回调在主循环执行。Delay 减至 0 后下一 tick 才触发，现有周期实际为 Period+1 tick。 */
void SCH_Update(void) //该函数由定时器中断触发，建议放入1ms定时器中断的时基中进行触发
{
	uint16_t Index;
	for(Index = 0; Index < SCH_MAX_TASKS; Index++)
	{
		if(SCH_tasks_G[Index].pTask)
		{
			if(SCH_tasks_G[Index].Delay == 0)
			{
				//任务需要运行
				SCH_tasks_G[Index].RunMe += 1;
				if(SCH_tasks_G[Index].Preiod)
				{
					//调度周期性的任务再次运行
					SCH_tasks_G[Index].Delay = SCH_tasks_G[Index].Preiod;
				}
			}
			else
			{
				//还没准备好运行，延迟减去1
				SCH_tasks_G[Index].Delay -= 1;
			}
		}
	}
}
/**
 * @brief   SCH_Add_Task：登记任务、初始延迟及周期
 * @param   pFunction:添加的任务函数  Delay:初始第一次延迟时间   Period:周期延迟时间
 * @return  返回任务的位置（以便以后删除）  
 * @note    none
 * 
 * 详细说明：此函数为任务添加函数，在main函数中，while(1)之前放置，通常上电只执行一次，用于将任务添加到进程中。
 */
uint16_t SCH_Add_Task(void(*pFunction)(void), const uint16_t Delay, const uint16_t Period)
{
	uint16_t Index = 0;
	   while((SCH_tasks_G[Index].pTask != 0) && (Index < SCH_MAX_TASKS))
	{
		Index++;
	}
	//是否到达队列结尾？
	if(Index == SCH_MAX_TASKS)
	{
		//任务队列已满
		//
		//设置全局错误变量
		Error_Code_G = ERROR_SCH_TOO_MANY_TASKS;
		return SCH_MAX_TASKS;
	}
	//如果运行到这里说明任务队列中有空间
	SCH_tasks_G[Index].pTask = pFunction;
	SCH_tasks_G[Index].Delay = Delay;
	SCH_tasks_G[Index].Preiod = Period;
	SCH_tasks_G[Index].RunMe = 0;
	return Index; 
}
/**
 * @brief   SCH_Dispatch_Tasks函数功能简述
 * @param   none
 * @return  none 
 * @note    none
 * 
 * 详细说明：此函数为任务刷新执行函数，在main函数中，while(1)中放置，会被持续重复执行，用于对任务的持续周期性调度执行。
 */
void SCH_Dispatch_Tasks(void)
{
	uint16_t Index;
	//调度（运行）下一个任务（如果有任务就绪）
	for(Index = 0; Index < SCH_MAX_TASKS; Index++)
	{
		if(SCH_tasks_G[Index].RunMe > 0)
		{
			(*SCH_tasks_G[Index].pTask)();
			SCH_tasks_G[Index].RunMe -= 1;
			//周期性的任务将自动再次执行
			//如果这是个单次执行的任务，将它从列表中删除
			if(SCH_tasks_G[Index].Preiod == 0)
			{
				SCH_Delete_Task(Index);
			}
		}
	}
	SCH_Report_Status();
	//保留的空闲接口；当前实现为空，不会进入硬件休眠
	SCH_Go_To_Sleep();
}
/**
 * @brief   SCH_Delete_Task函数功能简述
 * @param   Task_Index:任务索引号,用于定位是哪个任务
 * @return  错误代码:0 正常  1 错误
 * @note    none
 * 
 * 详细说明：此函数为任务删除函数，用于对进程中的任务进行删除。
 */
uint8_t SCH_Delete_Task(const uint16_t Task_Index)
{
	uint8_t Return_code;
	if(SCH_tasks_G[Task_Index].pTask == 0)
	{
		//这里没有任务...
		//
		//设置全局错误变量
		Error_Code_G = ERROR_SCH_CANOT_DELETE_TASK;
		//同时返回错误代码
		Return_code = SCH_RETURN_ERROR;
	}
	else
	{
		Return_code = SCH_RETURN_NORMAL;
	}
	SCH_tasks_G[Task_Index].pTask = 0x00000000;
	SCH_tasks_G[Task_Index].Delay = 0;
	SCH_tasks_G[Task_Index].Preiod = 0;
	SCH_tasks_G[Task_Index].RunMe = 0;
	return Return_code;
}
/**
 * @brief   SCH_Report_Status函数功能简述
 * @param   none
 * @return  none
 * @note    none
 * 
 * 详细说明：此函数为任务调度报告状态函数，用于对进程中的任务进行状态报告。
 */
void SCH_Report_Status(void)
{
	#ifdef SCH_REPORT_ERRORS
	//只在需要报告错误时适用
	//检查新的错误代码
	if(Error_Code_G != Last_error_code_G)
	{
		//LED错误输出
		Last_error_code_G = Error_Code_G;
		
		if(Error_Code_G != 0)
		{
			Error_tick_count_G = 60000;
		}
		else
		{
			Error_tick_count_G = 0;
		}
	}
	else
	{
		if(Error_tick_count_G != 0)
		{
			if(--Error_tick_count_G == 0)
			{
				Error_Code_G = NOT_ERROR;//复位错误
			}
		}
	}
	#endif
}
/**
 * @brief   SCH_Go_To_Sleep函数功能简述
 * @param   none
 * @return  none
 * @note    none
 * 
 * 详细说明：此函数仅为保留接口，当前为空实现；主循环仍持续轮询。
 */
static void SCH_Go_To_Sleep(void)
{

}

