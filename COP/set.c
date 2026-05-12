#include "config.h"
/**
 * @file    set.c
 * @brief   设置状态文件
 * @author  niu
 * @date    2026.02.26
 * @version 1.0
 * 
 * 详细描述:该文件实现了设置状态下，设备的运行情况，是状态机中设置情况下，设备的运行逻辑和情况。
 */
/***************************************************************************************************************************************************************/ 
/**
 * @brief   RtdTempSetControl函数功能简述
 * @param   none      
 * @return  none
 * @note    none
 * 
 * 详细说明：此函数为设备设置情况下的控制逻辑函数。
 */
void RtdTempSetControl(void)
{
     if(work_process_backups==start_up)
		 {
		    start_up_control();
		 }
		 else if(work_process_backups==run)
		 {
		     runing_control();
		 }
}

