/**
  ******************************************************************************
  * @file    shutdown.c
  * @author 
  * @version V1.0
  * @date
  * @brief   Shutdown state control implementation.
  ******************************************************************************
  */
#include "config.h"
/***************************************************************************************************************************************************************/ 
//static uint16_t ShutdownTimeFill;
/**
  * @function shutdown_control()
  * ------------------
  * @brief    Execute shutdown state control logic.
  * @param    None
  * @note     None
  */
void shutdown_control(void)
{

		 SystemErrState=NoErr;
}
/**
  * @function ShutdownTimeFillIrq()
  * ---------------------
  * @brief    Update shutdown timing counters from timer interrupt.
  * @param    None
  * @note     None
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
