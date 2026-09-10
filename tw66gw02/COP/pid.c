/**
  ******************************************************************************
  * @file    pid.c
  * @author 
  * @version V1.0
  * @date
  * @brief   PID controller implementation.
  ******************************************************************************
  */
#include "pid.h"
#include "config.h"
/***************************************************************************************************************************************************************/ 
PID_ValueStr PID;
uint16_t g_bPIDRunFlag = 0;
uint16_t iTemp=0;
uint16_t	Pid_up=0;
uint16_t	Pid_down=0;
uint16_t iTemp_shiyan=0;
uint16_t iTemp_Run=0;
uint16_t iTemp_RunLastValue=0;		   
uint16_t PID_Cycle=pid_run_cycle;
uint8_t  iTemp_Run_Times=0;
/**
  * @function PID_Init()
  * ------------
  * @brief    Initialize PID controller parameters.
  * @param    None
  * @note     None
  */
void PID_Init(void)
{
    PID.uKP_Coe=50;
    PID.uKI_Coe=20;
    PID.uKD_Coe=15;
    PID.iPriVal=0;
    PID.iCurVal=0;
    PID.liEkVal[2] = 0;
    PID.liEkVal[1] = 0;
    PID.liEkVal[0] = 0;
	
		g_bPIDRunFlag=PID_Cycle;
	
    Pid_down=0;Pid_up=100;
	
}
/**
  * @function PID_Operation()
  * ---------------
  * @brief    Calculate PID controller output.
  * @param    in_temp - input parameter
  * @note     None
  */
void PID_Operation(uint16_t in_temp)
{
    long Temp[3] = {0};
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
			
			Temp[0] = (signed long)PID.iSetVal - (signed long)PID.iCurVal;
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

			PID.iPriVal=iTemp*250;
			}
			else if (PID.iSetVal>=PID.iCurVal)	
			{
				iTemp=Pid_up+10;
	
				PID.iPriVal=0;
				PID.liEkVal[2] = 0;
				PID.liEkVal[1] = 0;
				PID.liEkVal[0] = 0;
				PID.iPriVal=iTemp*250;
			}
			else 
			{
				iTemp=Pid_down;
				PID.iPriVal=0;
				PID.liEkVal[2] = 0;
				PID.liEkVal[1] = 0;
				PID.liEkVal[0] = 0;
				PID.iPriVal=iTemp*250;
			}
			if(iTemp>=Pid_up) iTemp = Pid_up;
			if(iTemp<=Pid_down) iTemp = Pid_down;						
//			if(iTemp_Run_Times<=3) 
//			{
//			}
			
			iTemp_shiyan=iTemp;
			iTemp_Run=(float)(iTemp)*0.81;
}



