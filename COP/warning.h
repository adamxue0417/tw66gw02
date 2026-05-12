/***************************用于定义各种接口******************************/
#ifndef		__WARNING_H__
#define		__WARNING_H__
#include "config.h"

typedef enum _ErrState
{
  NoErr=0,
  ErrHot,//5秒内无反馈信号，则报警
  ErrMot,//5秒内无反馈信号，则报警
  ErrFan,//5秒内无反馈信号，则报警
  ErrP,//炉温探针短路或开路异常，则报警
  ErrL,	 //13分钟内点火不成功，则报警
  ErrH,	 //炉温探针无异常时，炉温温度值大于615F，则报警
	ErrBoardTemp,//板温错误
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




