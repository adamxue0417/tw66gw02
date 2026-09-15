/**
  ******************************************************************************
  * @file    start_up.c
  * @author 
  * @version V1.0
  * @date
  * @brief   历史启动接口；本文件各控制函数为空实现，实际初始化见 main.c 和 SysDataInit。
  ******************************************************************************
  */
#include "config.h"
/***************************************************************************************************************************************************************/ 
int StartUpStartTemp=0,StartUpRiseTemp=0;
uint16_t StartUpTimeFill=0;
int Thermocouple1Backup=0;
uint16_t StartUpKeepTempTimer=0;
//#define ThermocoupleWarningValue    120
/**
  * @function ResetStartUpControl()
  * ---------------------
  * @brief    保留接口；当前未执行计数复位。
  * @param    None
  * @note     None
  */
void ResetStartUpControl(void)
{
//		ignitor_success_flag=0;
//		StartUpTimeFill=0;
//	  StartUpKeepTempTimer=pid_run_cycle;
}
/**
  * @function start_up_control()
  * ------------------
  * @brief    保留接口；当前无启动控制逻辑。
  * @param    None
  * @note     None
  */
void start_up_control(void)
{

}
/**
  * @function StartUpTimeFillIrq()
  * --------------------
  * @brief    历史中断接口；计时逻辑已注释，当前不生效。
  * @param    None
  * @note     None
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





