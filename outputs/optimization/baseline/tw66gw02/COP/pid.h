/**
  ******************************************************************************
  * @file    pid.h
  * @author 
  * @version V1.0
  * @date
  * @brief   PID controller interface definitions.
  ******************************************************************************
  */
#ifndef		__PID_H__
#define		__PID_H__
#include "config.h"
typedef struct PID_Value
{
    long liEkVal[3];
    uint8_t uEkFlag[3];
    uint8_t uKP_Coe;
    uint8_t uKI_Coe;
    uint8_t uKD_Coe;
    long iPriVal;
    uint16_t iSetVal;
    uint16_t iCurVal;
}PID_ValueStr;

extern PID_ValueStr PID;
extern uint16_t iTemp;
extern uint16_t	Pid_up;
extern uint16_t	Pid_down;
extern uint16_t g_bPIDRunFlag;
extern uint16_t iTemp_shiyan;
extern uint16_t iTemp_Run;
extern uint16_t PID_Cycle;
extern uint16_t iTemp_RunLastValue;	

void PID_Init(void);
void PID_Operation(uint16_t in_temp);
void PidTimeFillIrq(void);
#endif
