/**
  ******************************************************************************
  * @file    shutdown.h
  * @author 
  * @version V1.0
  * @date
  * @brief   Shutdown state control interface definitions.
  ******************************************************************************
  */
#ifndef		__SHUTDOWN_H__
#define		__SHUTDOWN_H__
#include "config.h"
void shutdown_control(void);
void ShutdownTimeFillIrq(void);
#endif

