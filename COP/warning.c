#include "config.h"
/**
 * @file    warning.c
 * @brief   报警状态文件
 * @author  niu
 * @date    2026.02.26
 * @version 1.0
 * 
 * 详细描述:该文件实现了报警状态下，设备的运行情况，是状态机中报警情况下，设备的运行逻辑和情况。
 */
/***************************************************************************************************************************************************************/ 
ErrState SystemErrState;
/**
 * @brief   WarningDetect函数功能简述
 * @param   system_data 系统数据      
 * @return  none
 * @note    none
 * 
 * 详细说明：此函数为设备报警检测函数。
 */
void WarningDetect(void)
{	
//	  static uint8_t ErH_timer=0;		
//		//还少一个ErL报警，在StartUpControl中
//	 if(system_data.rtd_temp==HaveTempErr) 
//	 {
//				 work_process=warning;SystemErrState=ErrP;
//	 }
//	 else if(system_data.pt1000_temp[0]==HaveTempErr)	
//	 {
//				 work_process=warning;SystemErrState=ErrThermocouple1Unplug;	 
//	 } 
//	 else if(system_data.rtd_temp>WarningTemp_ErH)
//	 {
//		 ErH_timer++;
//		 if(ErH_timer>=50)
//		 {
//			 ErH_timer=0;
//			 work_process=warning;SystemErrState=ErrH;
//		 }
//	 }
////	 else if((system_data.mot_frequency==0)&&(system_data.mot_run_state==1))
////	 {
////		   Mot_timer++;
////		   if(Mot_timer>=50)
////			 {
////			    Mot_timer=0;
////				  work_process=warning;SystemErrState=ErrMot; 
////			 }    
////	 }
//	 else
//	 {
//					ErH_timer=0;	
//					//Mot_timer=0;	
//	 }
}
/**
 * @brief   WarningControl函数功能简述
 * @param   none      
 * @return  none
 * @note    none
 * 
 * 详细说明：此函数为设备报警情况下的控制逻辑函数。
 */
void WarningControl(void)
{

}
/**
 * @brief   WarningTimeFillIrq函数功能简述
 * @param   none
 * @return  none
 * @note    none
 * 
 * 详细说明：此函数为设备报警情况下的计时逻辑函数，放置于外部中断中用于精准计时。
 */
void WarningTimeFillIrq(void)
{

}

/* ========================================================================== */
/* 本地状态变量                                                              */
/* ========================================================================== */

/* 高温告警 */
//static uint8_t s_highTempAlarmActive = 0;
//static uint16_t s_highTempBeepCounter = 0;

///* 低温告警 */
//static uint8_t s_lowTempAlarmActive = 0;
//static uint16_t s_lowTempBeepCounter = 0;

///* 低电量告警 */
//static uint8_t s_lowBatteryAlarmActive = 0;

///* 探针错误 */
//static uint8_t s_probeErrorActive = 0;

/* ========================================================================== */
/* 温度告警实现                                                              */
/* ========================================================================== */

//void Task_HandleHighTempAlarm(uint16_t current_temp, uint16_t temp_high_limit)
//{
//    /**
//     * 高温告警处理:
//     * 1. 如果还未激活，启动告警显示和闪烁
//     * 2. 保持告警状态，直到温度恢复
//     * 
//     * 显示: "-HI" + 2Hz 闪烁
//     * 蜂鸣: 每2秒一次(由 AlarmPeriodic100ms 管理)
//     */
//    
//    if (current_temp > temp_high_limit)
//    {
//        if (!s_highTempAlarmActive)
//        {
//            /* 第一次触发告警 */
//            ScreenNew_TempHighFlash();    /* 显示 "-HI" 并启动2Hz闪烁 */
//            s_highTempAlarmActive = 1;
//            s_highTempBeepCounter = 0;
//            
//            /* 可以在这里添加蓝牙通知 */
//            /* BT_SendWarning(BT_HIGH_TEMP); */
//        }
//    }
//}

//void Task_HandleLowTempAlarm(uint16_t current_temp, uint16_t temp_low_limit)
//{
//    /**
//     * 低温告警处理:
//     * 1. 如果还未激活，启动告警显示和闪烁
//     * 2. 保持告警状态，直到温度恢复
//     * 
//     * 显示: "-LO" + 1Hz 闪烁
//     * 蜂鸣: 间歇蜂鸣(由 AlarmPeriodic100ms 管理)
//     */
//    
//    if (current_temp < temp_low_limit)
//    {
//        if (!s_lowTempAlarmActive)
//        {
//            /* 第一次触发告警 */
//            ScreenNew_TempLowFlash();     /* 显示 "-LO" 并启动1Hz闪烁 */
//            s_lowTempAlarmActive = 1;
//            s_lowTempBeepCounter = 0;
//            
//            /* 可以在这里添加蓝牙通知 */
//            /* BT_SendWarning(BT_LOW_TEMP); */
//        }
//    }
//}

//void Task_ClearHighTempAlarm(void)
//{
//    /**
//     * 清除高温告警
//     * - 停止闪烁
//     * - 恢复正常显示(温度值)
//     * - 停止蜂鸣
//     */
//    if (s_highTempAlarmActive)
//    {
//        ScreenNew_StopTempHighFlash();
//        s_highTempAlarmActive = 0;
//        s_highTempBeepCounter = 0;
//    }
//}

//void Task_ClearLowTempAlarm(void)
//{
//    /**
//     * 清除低温告警
//     * - 停止闪烁
//     * - 恢复正常显示(温度值)
//     * - 停止蜂鸣
//     */
//    if (s_lowTempAlarmActive)
//    {
//        ScreenNew_StopTempLowFlash();
//        s_lowTempAlarmActive = 0;
//        s_lowTempBeepCounter = 0;
//    }
//}

/* ========================================================================== */
/* 电池告警实现                                                              */
/* ========================================================================== */

//void Task_HandleLowBatteryAlarm(uint16_t battery_voltage)
//{
//    /**
//     * 低电量告警处理:
//     * 1. 在屏幕上显示低电量符号
//     * 2. 启动1Hz闪烁
//     * 3. 发送蓝牙通知(可选)
//     * 
//     * 触发条件: battery_voltage < 1.2v
//     */
//    
//    if (battery_voltage < 1200)
//    {
//        if (!s_lowBatteryAlarmActive)
//        {
//            ScreenNew_BatteryLowFlash();   /* 启动低电量闪烁 */
//            s_lowBatteryAlarmActive = 1;
//            
//            /* 可以在这里添加蓝牙通知 */
//            /* BT_SendWarning(BT_LOW_BATTERY); */
//        }
//    }
//}

//void Task_ClearLowBatteryAlarm(void)
//{
//    /**
//     * 清除低电量告警
//     * - 停止闪烁
//     * - 关闭低电量符号
//     */
//    if (s_lowBatteryAlarmActive)
//    {
//        ScreenNew_StopBatteryLowFlash();
//        s_lowBatteryAlarmActive = 0;
//    }
//}

/* ========================================================================== */
/* 探针错误实现                                                              */
/* ========================================================================== */

//void Task_HandleProbeError(uint8_t error_code)
//{
//    /**
//     * 处理探针相关错误:
//     * 0 = 探针未连接 → 显示 "---"
//     * 1 = 连接错误 → 显示 "---" + 错误指示
//     * 2 = 校准失败 → 显示校准失败提示
//     */
//    
//    if (!s_probeErrorActive)
//    {
//        switch(error_code)
//        {
//            case 0:  /* 未连接 */
//                ScreenNew_ShowNoProbe();
//                break;
//                
//            case 1:  /* 连接错误 */
//                ScreenNew_ProbeError();
//                break;
//                
//            case 2:  /* 校准失败 */
//                ScreenNew_ProbeCalibrationError();
//                break;
//                
//            default:
//                break;
//        }
//        
//        s_probeErrorActive = 1;
//    }
//}

//void Task_ClearProbeError(void)
//{
//    /**
//     * 清除探针错误显示
//     */
//    if (s_probeErrorActive)
//    {
//        ScreenNew_ProbeConnected();
//        s_probeErrorActive = 0;
//    }
//}

/* ========================================================================== */
/* 周期告警检查                                                              */
/* ========================================================================== */

void Task_AlarmPeriodic100ms(void)
{

//    if (s_highTempAlarmActive)
//    {
//        s_highTempBeepCounter++;
//        
//        if (s_highTempBeepCounter >= 20) 
//        {
//            s_highTempBeepCounter = 0;
//           
//        }
//    }
//    
//  
//    if (s_lowTempAlarmActive)
//    {
//        s_lowTempBeepCounter++;
//        
//        if (s_lowTempBeepCounter >= 20)  
//        {
//            s_lowTempBeepCounter = 0;
//        }
//        
//        if (s_lowTempBeepCounter < 8)  
//        {
//            
//        }
//    }
}
