#include "config.h"
/**
 * @file    idle.c
 * @brief   空闲状态文件
 * @author  niu
 * @date    2026.02.26
 * @version 1.0
 * 
 * 详细描述:该文件实现了空闲状态下，设备的运行情况，是状态机中空闲情况下，设备的运行逻辑和情况。
 */
/***************************************************************************************************************************************************************/ 
/**
 * @brief   idle_control函数功能简述
 * @param   none      
 * @return  none
 * @note    none
 * 
 * 详细说明：此函数为设备空闲情况下的控制逻辑函数。
 */
void idle_control(void)
{

//		 fan_turn_off();						     							//风扇关闭 
//		 all_heats_off();					                    //加热棒关闭 
		 SystemErrState=NoErr;											  //清除错误

}





