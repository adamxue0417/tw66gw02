/**
  ******************************************************************************
  * @file    buzz.c
  * @author 
  * @version V1.0
  * @date
  * @brief   Buzzer driver implementation.
  ******************************************************************************
  */
#include "buzz.h"
#include "config.h"
/***************************************************************************************************************************************************************/ 
/**
  * @function buzzer()
  * ------------
  * @brief    Control buzzer output timing.
  * @param    buzzer_control - input parameter
  * @note     None
  */
void buzzer(uint8_t buzzer_control)
{
       if(buzzer_control){buzzer_on_gpio;}
       else{buzzer_off_gpio;}
}

