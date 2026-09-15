/**
  ******************************************************************************
  * @file    shutdown.c
  * @author 
  * @version V1.0
  * @date
  * @brief   历史关机接口；实际蓝牙断电和电源锁存控制在 MainControl。
  ******************************************************************************
  */
#include "config.h"
/***************************************************************************************************************************************************************/ 
//static uint16_t ShutdownTimeFill;
/**
  * @function shutdown_control()
  * ------------------
  * @brief    仅清除系统错误状态，不执行硬件断电。
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
  * @brief    历史计时接口；内部逻辑已注释，当前不生效。
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
