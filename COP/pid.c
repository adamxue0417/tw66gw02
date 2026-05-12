#include "config.h"
/**
 * @file    pid.c
 * @brief   pid算法文件
 * @author  niu
 * @date    2026.02.26
 * @version 1.0
 * 
 * 详细描述:该文件为PID算法文件，用于设备恒温控制使用。
 */
/***************************************************************************************************************************************************************/ 
PID_ValueStr PID;               //定义一个结构体，这个结构体用来存算法中要用到的各种数据
uint16_t g_bPIDRunFlag = 0;          //PID运行标志位，PID算法不是一直在运算。而是每隔一定时间，算一次。
uint16_t iTemp=0;		   //马达要执行的百分百比0~100
uint16_t	Pid_up=0;		     //pid执行的上下限
uint16_t	Pid_down=0;		   //pid执行的上下限
uint16_t iTemp_shiyan=0;
uint16_t iTemp_Run=0;		   //由百分比计算要运行的时间
uint16_t iTemp_RunLastValue=0;		   
uint16_t PID_Cycle=pid_run_cycle;	//pid运行周期 s
uint8_t  iTemp_Run_Times=0;//进入PID运行的次数
/**
 * @brief   PID_Init函数功能简述
 * @param   none      
 * @return  none
 * @note    none
 * 
 * 详细说明：此函数为PID算法初始化文件，为PID算法使用恒温控制做准备。
 */
void PID_Init(void)
{
    PID.uKP_Coe=50;                       //比例系数
    PID.uKI_Coe=20;                       //积分常数
    PID.uKD_Coe=15;                       //微分常数（为0不动）
    PID.iPriVal=0;                        //上一时刻值（为0不动）
    PID.iCurVal=0;                        //实际值（为0不动）
    PID.liEkVal[2] = 0;
    PID.liEkVal[1] = 0;
    PID.liEkVal[0] = 0;
	
		g_bPIDRunFlag=PID_Cycle;
	
    Pid_down=0;Pid_up=100;
	
}
/* ********************************************************
 函数名称：PID_Operation()                                  
 函数功能：PID运算                    
入口参数：当前温度                      
 出口参数：无（隐形输出，U(k)）
 函数说明：U(k)+KP*[E(k)-E(k-1)]+KI*E(k)+KD*[E(k)-2E(k-1)+E(k-2)]                                      
******************************************************** */
/**
 * @brief   PID_Operation函数功能简述
 * @param   in_temp 当前温度      
 * @return  none
 * @note    none
 * 
 * 详细说明：此函数为PID算法操作函数，用于控制温度的恒定。
   算法公式：U(k)+KP*[E(k)-E(k-1)]+KI*E(k)+KD*[E(k)-2E(k-1)+E(k-2)]  
 */
void PID_Operation(uint16_t in_temp)
{
    long Temp[3] = {0};   //中间临时变量
    static uint16_t j=0;
    static uint16_t k=0;

	if(iTemp_Run_Times<=3) iTemp_Run_Times++;

	PID.iCurVal=in_temp;
	PID.iSetVal=system_data.InTempSet;
 

	if(PID.iSetVal>=PID.iCurVal) 	
	{
		j=PID.iSetVal - PID.iCurVal;
		k=300;
//	    Ctr_Hot = 1;

	}
	else 
	{
//		Ctr_Hot = 1;
		k=PID.iCurVal - PID.iSetVal;
		j=300;
	}


		   if(j<150||k<150)
		   {
			
			Temp[0] = (signed long)PID.iSetVal - (signed long)PID.iCurVal;    //偏差<=10,计算E(k)
            /* 数值进行移位，注意顺序，否则会覆盖掉前面的数值 */
            PID.liEkVal[2] = PID.liEkVal[1];
            PID.liEkVal[1] = PID.liEkVal[0];
            PID.liEkVal[0] = Temp[0];
            /* =================================================================== */
            Temp[0] = PID.liEkVal[0] - PID.liEkVal[1];  //E(k)-E(k-1)
			      Temp[2] = PID.liEkVal[0] - PID.liEkVal[1]*2 +  PID.liEkVal[2]; //E(k)-2E(k-1)+E(k-2)

            Temp[0] = (signed long)PID.uKP_Coe * Temp[0];        //KP*[E(k)-E(k-1)]
            Temp[1] = (signed long)PID.uKI_Coe * PID.liEkVal[0]; //KI*E(k)
            Temp[2] = (signed long)PID.uKD_Coe * Temp[2]; //KD*[E(k)-2E(k-1)+E(k-2)


			Temp[0]=Temp[0]+Temp[1]+Temp[2];
			PID.iPriVal=Temp[0]+PID.iPriVal;
			
			if(PID.iPriVal>0)
			{			 
	       iTemp=PID.iPriVal/250;
				if(iTemp<=Pid_down) iTemp = Pid_down;
				if(iTemp>=Pid_up) iTemp = Pid_up;
			}   
			else  iTemp=Pid_down;

			PID.iPriVal=iTemp*250;//用于下次PID计算，iTemp为实际执行量，PID.iPriVal为计算的量
			}
			else if (PID.iSetVal>=PID.iCurVal)	
			{
				iTemp=Pid_up+10;
	
				PID.iPriVal=0;
				PID.liEkVal[2] = 0;
				PID.liEkVal[1] = 0;
				PID.liEkVal[0] = 0;
				PID.iPriVal=iTemp*250;//用于下次PID计算，iTemp为实际执行量，PID.iPriVal为计算的量
			}
			else 
			{
				iTemp=Pid_down;
				PID.iPriVal=0;
				PID.liEkVal[2] = 0;
				PID.liEkVal[1] = 0;
				PID.liEkVal[0] = 0;
				PID.iPriVal=iTemp*250;//用于下次PID计算，iTemp为实际执行量，PID.iPriVal为计算的量
			}
			if(iTemp>=Pid_up) iTemp = Pid_up;
			if(iTemp<=Pid_down) iTemp = Pid_down;						
//			if(iTemp_Run_Times<=3) 
//			{
//				iTemp=Pid_up-10;//前3此计算的值不用
//				PID.iPriVal=iTemp*250;//用于下次PID计算，iTemp为实际执行量，PID.iPriVal为计算的量
//			}
			
			iTemp_shiyan=iTemp;
//			iTemp_Run=iTemp*PID_Cycle/10;//秒，扩大了10倍
			iTemp_Run=(float)(iTemp)*0.81;//秒   最大停止时间120S
}



