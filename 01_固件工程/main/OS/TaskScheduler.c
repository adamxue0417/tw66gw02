#include "TaskScheduler.h"
#include "TaskScheduler_config.h"
#include "stm32f0xx_hal.h"
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
volatile sTask SCH_tasks_G[SCH_MAX_TASKS];  //任务队列
volatile SCH_Error_TypeDef Error_Code_G = NOT_ERROR;
static uint32_t s_error_started_ms;//跟踪上次记录错误以来的时间
static uint8_t Last_error_code_G = NOT_ERROR;//上次的错误代码（在1 分钟之后复位）
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
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    for (i = 0u; i < SCH_MAX_TASKS; i++) {
        SCH_tasks_G[i].pTask = 0;
        SCH_tasks_G[i].Delay = 0u;
        SCH_tasks_G[i].Preiod = 0u;
        SCH_tasks_G[i].RunMe = 0u;
    }
    Error_Code_G = NOT_ERROR;
    Last_error_code_G = NOT_ERROR;
    s_error_started_ms = HAL_GetTick();
    __set_PRIMASK(primask);
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
				if (SCH_tasks_G[Index].RunMe < UINT8_MAX) {
                    SCH_tasks_G[Index].RunMe += 1u;
                } else {
                    /* Do not wrap pending work to zero and incorrectly sleep. */
                    Error_Code_G = ERROR_SCH_PENDING_OVERFLOW;
                }
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
 * @brief   SCH_Update函数功能简述
 * @param   pFunction:添加的任务函数  Delay:初始第一次延迟时间   Period:周期延迟时间
 * @return  返回任务的位置（以便以后删除）  
 * @note    none
 * 
 * 详细说明：此函数为任务添加函数，在main函数中，while(1)之前放置，通常上电只执行一次，用于将任务添加到进程中。
 */
uint16_t SCH_Add_Task(void(*pFunction)(void), const uint16_t Delay, const uint16_t Period)
{
    uint16_t index = 0u;
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    while ((index < SCH_MAX_TASKS) && (SCH_tasks_G[index].pTask != 0)) { index++; }
    if ((index == SCH_MAX_TASKS) || (pFunction == 0)) {
        Error_Code_G = ERROR_SCH_TOO_MANY_TASKS;
        __set_PRIMASK(primask);
        return SCH_MAX_TASKS;
    }
    SCH_tasks_G[index].Delay = Delay;
    SCH_tasks_G[index].Preiod = Period;
    SCH_tasks_G[index].RunMe = 0u;
    SCH_tasks_G[index].pTask = pFunction;
    __set_PRIMASK(primask);
    return index;
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
    uint16_t index;
    for (index = 0u; index < SCH_MAX_TASKS; index++) {
        void (*task)(void) = 0;
        uint32_t primask = __get_PRIMASK();
        __disable_irq();
        if ((SCH_tasks_G[index].pTask != 0) && (SCH_tasks_G[index].RunMe > 0u)) {
            task = SCH_tasks_G[index].pTask;
            /* Claim before running: an ISR during the callback cannot be lost. */
            SCH_tasks_G[index].RunMe -= 1u;
            if (SCH_tasks_G[index].Preiod == 0u) {
                (void)SCH_Delete_Task(index);
            }
        }
        __set_PRIMASK(primask);
        if (task != 0) { task(); }
    }
    SCH_Report_Status();
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
    uint8_t result = SCH_RETURN_NORMAL;
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    if (Task_Index >= SCH_MAX_TASKS) {
        Error_Code_G = ERROR_SCH_CANOT_DELETE_TASK;
        __set_PRIMASK(primask);
        return SCH_RETURN_ERROR;
    }
    if (SCH_tasks_G[Task_Index].pTask == 0) {
        Error_Code_G = ERROR_SCH_CANOT_DELETE_TASK;
        result = SCH_RETURN_ERROR;
    }
    SCH_tasks_G[Task_Index].pTask = 0;
    SCH_tasks_G[Task_Index].Delay = 0u;
    SCH_tasks_G[Task_Index].Preiod = 0u;
    SCH_tasks_G[Task_Index].RunMe = 0u;
    __set_PRIMASK(primask);
    return result;
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
    uint32_t primask = __get_PRIMASK();
    uint32_t now_ms;
    __disable_irq();
    now_ms = HAL_GetTick();
    if (Error_Code_G != Last_error_code_G) {
        Last_error_code_G = (uint8_t)Error_Code_G;
        s_error_started_ms = now_ms;
    } else if ((Error_Code_G != NOT_ERROR) &&
               ((uint32_t)(now_ms - s_error_started_ms) >= SCH_ERROR_HOLD_MS)) {
        Error_Code_G = NOT_ERROR;
        Last_error_code_G = NOT_ERROR;
    }
    __set_PRIMASK(primask);
#endif
}
/**
 * @brief   SCH_Go_To_Sleep函数功能简述
 * @param   none
 * @return  none
 * @note    none
 * 
 * 详细说明：此函数为任务调度器进入休眠函数，用于对进程中的任务空闲时，对任务调度器进行休眠处理。
 */
void SCH_Go_To_Sleep(void)
{
#if SCH_IDLE_SLEEP_ENABLED
    uint16_t index;
    uint32_t primask = __get_PRIMASK();
    /* Sleep is only valid in thread mode with interrupts originally enabled. */
    if ((primask != 0u) || (__get_IPSR() != 0u)) { return; }
    __disable_irq();
    for (index = 0u; index < SCH_MAX_TASKS; index++) {
        if (SCH_tasks_G[index].RunMe != 0u) {
            __set_PRIMASK(primask);
            return;
        }
    }
    /* PM0215: a pending enabled IRQ wakes WFI even with PRIMASK set.
       Keep the mask across the final check/WFI, then service the IRQ.
       SysTick and all peripheral clocks remain enabled. */
    SCB->SCR &= ~(SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk);
    __DSB();
    __WFI();
    __set_PRIMASK(primask);
    __ISB();
#endif
}

