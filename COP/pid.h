/***************************用于定义各种接口******************************/
#ifndef		__PID_H__
#define		__PID_H__
#include "config.h"
typedef struct PID_Value
{
    long liEkVal[3];          //差值保存，给定和反馈的差值
    uint8_t uEkFlag[3];          //符号，1则对应的为负数，0为对应的为正数    
    uint8_t uKP_Coe;             //比例系数
    uint8_t uKI_Coe;             //积分常数
    uint8_t uKD_Coe;             //微分常数
    long iPriVal;             //上一时刻值
    uint16_t iSetVal;             //设定值
    uint16_t iCurVal;             //实际值
}PID_ValueStr;

extern PID_ValueStr PID;               //定义一个结构体，这个结构体用来存算法中要用到的各种数据
extern uint16_t iTemp;		   //马达要执行的百分百比0~100
extern uint16_t	Pid_up;		   //pid执行的上下限
extern uint16_t	Pid_down;		   //pid执行的上下限
extern uint16_t g_bPIDRunFlag;          //PID运行标志位，PID算法不是一直在运算。而是每隔一定时间，算一次。
extern uint16_t iTemp_shiyan;
extern uint16_t iTemp_Run;		   //由百分比计算要运行的时间
extern uint16_t PID_Cycle;	//pid运行周期
extern uint16_t iTemp_RunLastValue;	

void PID_Init(void);
void PID_Operation(uint16_t in_temp);
void PidTimeFillIrq(void);
#endif
