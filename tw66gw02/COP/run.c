/**
  ******************************************************************************
  * @file    run.c
  * @author 
  * @version V1.0
  * @date
  * @brief   Run state control and PID timing implementation.
  ******************************************************************************
  */
#include "run.h"
#include "config.h"
/***************************************************************************************************************************************************************/ 
/**
  * @function runing_control()
  * ----------------
  * @brief    Execute run state control logic.
  * @param    None
  * @note     None
  */
void runing_control(void)	
{
//      uint8_t i;
//	    uint16_t j=0x0001;
//	    if(screen_data.DisplayMap.control_mode)
//			{
//					for(i=0;i<8;i++)
//					{
//							if(system_data.pt1000_temp[i]>screen_data.DisplayMap.set_temp[i]){heat_control(i,off);}
//							else{heat_control(i,on);}			
//					}			
//			}
//			else
//			{
//					for(i=0;i<8;i++)
//					{
//							if(screen_data.DisplayMap.mamal_control&j){
//							heat_control(i,on);
//							}
//							else{
//							heat_control(i,off);
//							}
//							j=j<<1;
//					}
//			}		
}
/**
  * @function PidTimeFillIrq()
  * ----------------
  * @brief    Update PID timing counters from timer interrupt.
  * @param    None
  * @note     None
  */
void PidTimeFillIrq(void)
{
//			if(work_process==run)
//			{
//					g_bPIDRunFlag++;
//			}
//			else if(work_process==set){}
//			else
//			{
//					g_bPIDRunFlag=PID_Cycle;

//			}
}



