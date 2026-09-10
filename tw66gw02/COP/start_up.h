/**
  ******************************************************************************
  * @file    start_up.h
  * @author 
  * @version V1.0
  * @date
  * @brief   Startup state control interface definitions.
  ******************************************************************************
  */
#ifndef		__START_UP_H__
#define		__START_UP_H__
#include <stdint.h>
void start_up_control(void);
void ResetStartUpControl(void);
void StartUpTimeFillIrq(void);
#endif
