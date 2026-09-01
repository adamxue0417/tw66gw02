/**
  ******************************************************************************
  * @file    start_up.c
  * @author 
  * @version V1.0
  * @date
  * @brief   Startup state control implementation.
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
  * @brief    Reset startup state control counters.
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
  * @brief    Execute startup state control logic.
  * @param    None
  * @note     None
  */
void start_up_control(void)
{

}
/**
  * @function StartUpTimeFillIrq()
  * --------------------
  * @brief    Update startup timing counters from timer interrupt.
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





