/**
  ******************************************************************************
  * @file    warning.h
  * @author 
  * @version V1.0
  * @date
  * @brief   Warning state and alarm interface definitions.
  ******************************************************************************
  */
#ifndef		__WARNING_H__
#define		__WARNING_H__
#include "config.h"

typedef enum _ErrState
{
  NoErr=0,
  ErrHot,
  ErrMot,
  ErrFan,
  ErrP,
  ErrL,
  ErrH,
	ErrBoardTemp,
	ErrThermocouple2,
	ErrThermocouple1Unplug,
	ErrThermocouple2Unplug,
	ErrLowbattery
	
}ErrState;		
extern ErrState SystemErrState;
void WarningControl(void);
void WarningTimeFillIrq(void);
void WarningDetect(void);
#endif




