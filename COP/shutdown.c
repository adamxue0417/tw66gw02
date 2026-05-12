#include "config.h"
/**
 * @file    shutdown.c
 * @brief   关机状态文件
 * @author  niu
 * @date    2026.02.26
 * @version 1.0
 * 
 * 详细描述:该文件实现了关机状态下，设备的运行情况，是状态机中关机情况下，设备的运行逻辑和情况。
 */
/***************************************************************************************************************************************************************/ 
//static uint16_t ShutdownTimeFill;
/**
 * @brief   shutdown_control函数功能简述
 * @param   none      
 * @return  none
 * @note    none
 * 
 * 详细说明：此函数为设备关机情况下的控制逻辑函数。
 */
void shutdown_control(void)
{

		 SystemErrState=NoErr; /*清除错误*/
}
/**
 * @brief   ShutdownTimeFillIrq函数功能简述
 * @param   none
 * @return  none
 * @note    none
 * 
 * 详细说明：此函数为设备关机情况下的计时逻辑函数，放置于外部中断中用于精准计时。
 */
void ShutdownTimeFillIrq(void)
{
//	static uint16_t Timer1SCounter=0;
//	if(work_process==shutdown)
//	{
//			if(Timer1SCounter>=1000)
//			{Timer1SCounter=0;if(ShutdownTimeFill>0){ShutdownTimeFill--;}}
//			else
//			{	Timer1SCounter++;}		
//	}
//	else
//	{
//	    Timer1SCounter=0;ShutdownTimeFill=ShutdownTime;
//	} 

}
