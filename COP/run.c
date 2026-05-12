#include "config.h"
/**
 * @file    run.c
 * @brief   运行状态文件
 * @author  niu
 * @date    2026.02.26
 * @version 1.0
 * 
 * 详细描述:该文件实现了运行状态下，设备的运行情况，是状态机中运行情况下，设备的运行逻辑和情况。
 */
/***************************************************************************************************************************************************************/ 
/**
 * @brief   runing_control函数功能简述
 * @param   none      
 * @return  none
 * @note    none
 * 
 * 详细说明：此函数为设备运行情况下的控制逻辑函数。
 */
void runing_control(void)	
{
//      uint8_t i;
//	    uint16_t j=0x0001;
//	    fan_turn_on();																//风扇开启
//	    if(screen_data.DisplayMap.control_mode)
//			{
//				  //自动控制
//					for(i=0;i<8;i++)
//					{
//							if(system_data.pt1000_temp[i]>screen_data.DisplayMap.set_temp[i]){heat_control(i,off);}
//							else{heat_control(i,on);}			
//					}			
//			}
//			else
//			{
//				  //手动控制
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
 * @brief   PidTimeFillIrq函数功能简述
 * @param   none
 * @return  none
 * @note    none
 * 
 * 详细说明：此函数为设备运行情况下的PID计时逻辑函数，放置于外部中断中用于精准计时。
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



