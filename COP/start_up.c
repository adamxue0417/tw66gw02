#include "config.h"
/**
 * @file    start_up.c
 * @brief   启动状态文件
 * @author  niu
 * @date    2026.02.26
 * @version 1.0
 * 
 * 详细描述:该文件实现了启动状态下，设备的运行情况，是状态机中启动情况下，设备的运行逻辑和情况。
 */
/***************************************************************************************************************************************************************/ 
int StartUpStartTemp=0,StartUpRiseTemp=0;
uint16_t StartUpTimeFill=0;
int Thermocouple1Backup=0;
uint16_t StartUpKeepTempTimer=0;
//#define ThermocoupleWarningValue    120
/**
 * @brief   ResetStartUpControl函数功能简述
 * @param   none      
 * @return  none
 * @note    none
 * 
 * 详细说明：此函数为设备启动情况下的运行控制复位函数，让设备在启动阶段各参数恢复初始默认状态。
 */
void ResetStartUpControl(void)
{
//		ignitor_success_flag=0;
//		StartUpTimeFill=0;
//	  StartUpKeepTempTimer=pid_run_cycle;
}
/**
 * @brief   start_up_control函数功能简述
 * @param   none      
 * @return  none
 * @note    none
 * 
 * 详细说明：此函数为设备启动情况下的控制逻辑函数。
 */
void start_up_control(void)
{

}
/**
 * @brief   StartUpTimeFillIrq函数功能简述
 * @param   none
 * @return  none
 * @note    none
 * 
 * 详细说明：此函数为设备启动情况下的计时逻辑函数，放置于外部中断中用于精准计时。
 */
void StartUpTimeFillIrq(void)
{
//	static uint16_t Timer1SCounter=0;
//	if(work_process==start_up)
//	{
//	   	if(Timer1SCounter>=1000){Timer1SCounter=0;{StartUpTimeFill++;}}
//			else{	Timer1SCounter++;}	
//			
//			if(ignitor_success_flag){StartUpKeepTempTimer++;}else{StartUpKeepTempTimer=0;}
//			
//	}
//	else if(work_process==set){}
//	else
//	{
//	    Timer1SCounter=0;ResetStartUpControl(); 
//	}

}





