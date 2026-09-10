/**
  ******************************************************************************
  * @file    warning.c
  * @author 
  * @version V1.0
  * @date
  * @brief   Warning state detection, warning control and alarm task implementation.
  ******************************************************************************
  */
#include "config.h"
/***************************************************************************************************************************************************************/ 
ErrState SystemErrState;
/**
  * @function WarningDetect()
  * ---------------
  * @brief    Detect warning conditions from current system data.
  * @param    None
  * @note     None
  */
void WarningDetect(void)
{	
//	  static uint8_t ErH_timer=0;		
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
  * @function WarningControl()
  * ----------------
  * @brief    Execute warning state control logic.
  * @param    None
  * @note     None
  */
void WarningControl(void)
{

}
/**
  * @function WarningTimeFillIrq()
  * --------------------
  * @brief    Update warning timing counters from timer interrupt.
  * @param    None
  * @note     None
  */
void WarningTimeFillIrq(void)
{

}

/* ========================================================================== */
/* ========================================================================== */

//static uint8_t s_highTempAlarmActive = 0;
//static uint16_t s_highTempBeepCounter = 0;

//static uint8_t s_lowTempAlarmActive = 0;
//static uint16_t s_lowTempBeepCounter = 0;

//static uint8_t s_lowBatteryAlarmActive = 0;

//static uint8_t s_probeErrorActive = 0;

/* ========================================================================== */
/* ========================================================================== */

//void Task_HandleHighTempAlarm(uint16_t current_temp, uint16_t temp_high_limit)
//{
//    /**
//     * 
//     */
//    
//    if (current_temp > temp_high_limit)
//    {
//        if (!s_highTempAlarmActive)
//        {
//            s_highTempAlarmActive = 1;
//            s_highTempBeepCounter = 0;
//            
//            /* BT_SendWarning(BT_HIGH_TEMP); */
//        }
//    }
//}

//void Task_HandleLowTempAlarm(uint16_t current_temp, uint16_t temp_low_limit)
//{
//    /**
//     * 
//     */
//    
//    if (current_temp < temp_low_limit)
//    {
//        if (!s_lowTempAlarmActive)
//        {
//            s_lowTempAlarmActive = 1;
//            s_lowTempBeepCounter = 0;
//            
//            /* BT_SendWarning(BT_LOW_TEMP); */
//        }
//    }
//}

//void Task_ClearHighTempAlarm(void)
//{
//    /**
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
//     */
//    if (s_lowTempAlarmActive)
//    {
//        ScreenNew_StopTempLowFlash();
//        s_lowTempAlarmActive = 0;
//        s_lowTempBeepCounter = 0;
//    }
//}

/* ========================================================================== */
/* ========================================================================== */

//void Task_HandleLowBatteryAlarm(uint16_t battery_voltage)
//{
//    /**
//     * 
//     */
//    
//    if (battery_voltage < 1200)
//    {
//        if (!s_lowBatteryAlarmActive)
//        {
//            s_lowBatteryAlarmActive = 1;
//            
//            /* BT_SendWarning(BT_LOW_BATTERY); */
//        }
//    }
//}

//void Task_ClearLowBatteryAlarm(void)
//{
//    /**
//     */
//    if (s_lowBatteryAlarmActive)
//    {
//        ScreenNew_StopBatteryLowFlash();
//        s_lowBatteryAlarmActive = 0;
//    }
//}

/* ========================================================================== */
/* ========================================================================== */

//void Task_HandleProbeError(uint8_t error_code)
//{
//    /**
//     */
//    
//    if (!s_probeErrorActive)
//    {
//        switch(error_code)
//        {
//                ScreenNew_ShowNoProbe();
//                break;
//                
//                ScreenNew_ProbeError();
//                break;
//                
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
//     */
//    if (s_probeErrorActive)
//    {
//        ScreenNew_ProbeConnected();
//        s_probeErrorActive = 0;
//    }
//}

/* ========================================================================== */
/**
  * @function Task_AlarmPeriodic100ms()
  * -------------------------
  * @brief    Execute periodic 100 ms alarm processing.
  * @param    None
  * @note     None
  */
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
