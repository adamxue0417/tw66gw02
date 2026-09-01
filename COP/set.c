/**
  ******************************************************************************
  * @file    set.c
  * @author 
  * @version V1.0
  * @date
  * @brief   Set state control implementation.
  ******************************************************************************
  */
#include "config.h"
/***************************************************************************************************************************************************************/ 
/**
  * @function RtdTempSetControl()
  * -------------------
  * @brief    Execute RTD temperature set-state control logic.
  * @param    None
  * @note     None
  */
void RtdTempSetControl(void)
{
     if(work_process_backups==start_up)
		 {
		    start_up_control();
		 }
		 else if(work_process_backups==run)
		 {
		     runing_control();
		 }
}

