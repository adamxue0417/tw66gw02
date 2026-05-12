/**
  ******************************************************************************
  * @file    C8721.c
  * @author 
  * @version V1.0
  * @date
  * @brief   Command and communication driver level with C8721. 
  *          Functional configuration and low level display function.
  ******************************************************************************
  */
#include "C8721.h"
#include "screen_c8721.h"
/*
 * @brief  public variable define
 * @note
 */
ty_cmd_config_t tCmdConfig;
ty_cmd_setting_t tCmdSetting;
uint8 CF_DisplayBuf[CF_BUF_SIZE];

const uint8 BreathGamma[64] = { 0  ,1  ,2  ,3  ,4  ,5  ,6  ,7  ,
                                8  ,10 ,12 ,14 ,16 ,18 ,20 ,22 ,
                                24 ,26 ,29 ,32 ,35 ,38 ,41 ,44 ,
                                47 ,50 ,53 ,57 ,61 ,65 ,69 ,73 ,
                                77 ,81 ,85 ,89 ,94 ,99 ,104,109,
                                114,119,124,129,134,140,146,152,
                                158,164,170,176,182,188,195,202,
                                209,216,223,230,237,244,251,255 };

void		CF_TimeDlyUs(void)
{
	 uint16_t i=0;
		for(i=0;i<50;i++)
		{
				__NOP;
	  }
}
/**
  * @function CF_SetCurrentCommand()
  * -------------------------------
  * @brief    Configure the current setting command.
  * @param    None
  * @note     None
  */
static void CF_SetCurrentCommand()
{
    tCmdSetting.Current = tCmdConfig.emCurrentGain;
}

/**
  * @function CF_SetModeCommand()
  * ----------------------------
  * @brief    Configure the mode setting command.
  * @param    None
  * @note     None
  */
static void CF_SetModeCommand()
{
    tCmdSetting.Mode = (tCmdConfig.emClockMode << 7) + (tCmdConfig.emComScan << 2)
                       + (tCmdConfig.emWorkMode << 1) + tCmdConfig.emDataUpdata;
}

/**
  * @function CF_SetDisplayCommand()
  * -------------------------------
  * @brief    Configure the display setting command.
  * @param    None
  * @note     None
  */
static void CF_SetDisplayCommand()
{
    tCmdSetting.Display = (tCmdConfig.emOverT125Protect << 7) + (tCmdConfig.emPwmClock << 4)
                          + (tCmdConfig.emDisplay << 3) + (tCmdConfig.emGhostReduction << 2)
                          + tCmdConfig.emRowBreakTime;
}

/**
  * @function CF_SetControlCommand()
  * -------------------------------
  * @brief    Configure the control setting command.
  * @param    None
  * @note     None
  */
static void CF_SetControlCommand()
{
    tCmdSetting.Control = (tCmdConfig.emSleepFunction << 6) + (tCmdConfig.emAutoSleepFunction << 5) 
                          + (tCmdConfig.emGlobalReset << 4) + (tCmdConfig.emSoftReset << 3)                               
                          + (tCmdConfig.emOverT150Protect << 1) + tCmdConfig.emPowerDownReset;
}

/**
  * @function CF_SendByte()
  * ----------------------
  * @brief    Sending one byte to led driver.
  * @param    sdata - the sending data
  * @note     None
  */
static void CF_SendByte(uint8 sdata)
{
    uint8 i;

    for (i=0; i<8; i++)
    {
        CF_SCL_Write(0);
        if (sdata & 0x80)
            CF_SDA_Write(1);
        else
            CF_SDA_Write(0);
        CF_TimeDlyUs();
        CF_SCL_Write(1);
        CF_TimeDlyUs();
        sdata <<= 1;
    }
    CF_SCL_Write(0);
}

/**
  * @function CF_SendCLK()
  * ----------------------
  * @brief    Sending one CLK.
  * @param    None
  * @note     None
  */
static void CF_SendSCK()
{
    CF_SCL_Write(0);
    CF_TimeDlyUs();
    CF_SCL_Write(1);
    CF_TimeDlyUs();
    CF_SCL_Write(0);
}

/**
  * @function CF_SendCommandPackage()
  * --------------------------------
  * @brief    Sending command package to led driver.
  * @param    cmdType - command type
  * @note     None
  */
static void CF_SendCommandPackage(em_cmd_type_t cmdType)
{
    uint8 checkSum = 0;
    uint8 command;

    CF_SendByte(0x5A);
    CF_SendByte(0xFF);
    tCmdConfig.emCmdType = cmdType;
    command = tCmdConfig.emCmdType;
    CF_SendByte(command);
    checkSum = 0x5A + 0xFF + command;
    CF_SendByte(checkSum);
}

/**
  * @function CF_SendSetPackage()
  * ----------------------------
  * @brief    Sending set package to led driver.
  * @param    None
  * @note     None
  */
static void CF_SendSetPackage()
{
    uint8 checkSum = 0;

    CF_SendByte(tCmdSetting.Current);
    CF_SendByte(tCmdSetting.Mode);
    CF_SendByte(tCmdSetting.Display);
    CF_SendByte(tCmdSetting.Control);
    checkSum = tCmdSetting.Current + tCmdSetting.Mode
               + tCmdSetting.Display + tCmdSetting.Control;
    CF_SendByte(checkSum);
}

/**
  * @function CF_SendDataFrame()
  * ---------------------------
  * @brief    Sending data frame to led driver.
  * @param    *pDataBuf - point to data buffer
  * @note     None
  */
static void CF_SendDataFrame(uint8 *pDataBuf)
{
    uint8 checkSum = 0;
    uint16 i, dataLength = 0;

    dataLength = CF_SEG_NUM * (tCmdConfig.emComScan + 1);

    CF_SendCommandPackage(CmdCommand);
    CF_SendSetPackage();
    CF_SendCommandPackage(CmdData);

    checkSum = 0;
    for (i=0; i<dataLength; i++)
    {
        CF_SendByte(*(pDataBuf+i));
        checkSum += *(pDataBuf+i);
    }
    CF_SendByte(checkSum);
}

/**
  * @function CF_ConfigCommand()
  * ---------------------------
  * @brief    Config the command paramaters.
  * @param    None
  * @note     None
  */
void CF_ConfigCommand()
{
    tCmdConfig.emCmdType               = CmdCommand;              // command type 
    tCmdConfig.emCurrentGain           = mA_10;                   // current value  
    tCmdConfig.emClockMode             = InternalClock;           // internal clock mode
    tCmdConfig.emComScan               = Com8;                    // number of com ports in use
    tCmdConfig.emWorkMode              = NormalMode;              // work mode
    tCmdConfig.emDataUpdata            = AutomaticUpdate;         // data update mode, recommend automatic update
    tCmdConfig.emOverT125Protect       = Otp125Enable;            // temperature protection1 -125?  
    tCmdConfig.emPwmClock              = Clock1M;                 // pwm clock frequency
    tCmdConfig.emDisplay               = DisplayEnable;           // diaplay enable
    tCmdConfig.emGhostReduction        = Slight;                  // ghosting
    tCmdConfig.emRowBreakTime          = PwmCycle8;               // scanning interval
    tCmdConfig.emSleepFunction         = SleepDisable;            // sleep enable
    tCmdConfig.emAutoSleepFunction     = AutoSleepDisable;        // auto sleep disable
    tCmdConfig.emGlobalReset           = GlobalDisable;           // global reset (clear RAM)
    tCmdConfig.emSoftReset             = SoftDisable;             // soft reset (refresh display)     
    tCmdConfig.emOverT150Protect       = Otp150Enable;            // temperature protection1 -150?  
    tCmdConfig.emPowerDownReset        = PowerEnable;             // power-off reset  
}

/**
  * @function CF_ReconfigCommand()
  * -----------------------------
  * @brief    Reconfig the command paramaters.
  * @param    None
  * @note     None
  */
void CF_ReconfigCommand()
{          
    tCmdConfig.emCmdType               = CmdCommand;           
    tCmdConfig.emCurrentGain           = mA_10;                
    tCmdConfig.emClockMode             = InternalClock;        
    tCmdConfig.emComScan               = Com8;                 
    tCmdConfig.emWorkMode              = NormalMode;           
    tCmdConfig.emDataUpdata            = AutomaticUpdate;      
    tCmdConfig.emOverT125Protect       = Otp125Enable;         
    tCmdConfig.emPwmClock              = Clock8M;              
    tCmdConfig.emDisplay               = DisplayEnable;        
    tCmdConfig.emGhostReduction        = Strong;   
    tCmdConfig.emRowBreakTime          = PwmCycle8;            
    tCmdConfig.emSleepFunction         = SleepDisable;         
    tCmdConfig.emAutoSleepFunction     = AutoSleepDisable;     
    tCmdConfig.emGlobalReset           = GlobalDisable;        
    tCmdConfig.emSoftReset             = SoftDisable;            
    tCmdConfig.emOverT150Protect       = Otp150Enable;    
    tCmdConfig.emPowerDownReset        = PowerEnable; 

    CF_SetCurrentCommand();
    CF_SetModeCommand();
    CF_SetDisplayCommand();
    CF_SetControlCommand();
}

/**
  * @function CF_Init()
  * ------------------
  * @brief    Initlize all led drivers.
  * @param    None
  * @note     None
  */
void CF_Init(void)
{
    CF_SDA_Write(1);
    CF_SCL_Write(1);

    CF_ConfigCommand();
    CF_SetCurrentCommand();
    CF_SetModeCommand();
    CF_SetDisplayCommand();
    CF_SetControlCommand();
}

/**
  * @function CF_DisplayClearBuf()
  * -----------------------------
  * @brief    Clear all display buffer.
  * @param    None
  * @note     None
  */
void CF_DisplayClearBuf(void)
{
    uint16 i;
    uint16 dataLength = 0;

    dataLength = CF_SEG_NUM * (tCmdConfig.emComScan + 1);

    for (i=0; i<dataLength; i++)
        CF_DisplayBuf[i] = 0;
}

/**
  * @function CF_DisplayAll()
  * ------------------------
  * @brief    Display all led with special luminance.
  * @param    lumi - brightness value (0x00 ~ 0xFF)
  * @note     None
  */
void CF_DisplayAll(uint8 lumi)
{
    uint16 i;
    uint16 dataLength = 0;

    dataLength = CF_SEG_NUM * (tCmdConfig.emComScan + 1);

    for (i=0; i<dataLength; i++)
        CF_DisplayBuf[i] = lumi;

   // CF_SendDataFrame(CF_DisplayBuf);
   // CF_SendCommandPackage(CmdDataUpdate);
  //  CF_SendSCK();
}

/**
  * @function CF_DisplayBufAutomatic()
  * ---------------------------------
  * @brief    Write luminance of the display buffer to led
  *           driver, automatic update the display.
  * @param    None
  * @note     None
  */
void CF_DisplayBufAutomatic(void)
{
    CF_SendDataFrame(CF_DisplayBuf);
    CF_SendCommandPackage(CmdDataUpdate); 
    CF_SendSCK();
}


void  CF_DisplaySegment(uint8_t step)
{
	   uint8_t i,dataLength;
	   static uint8 checksum=0;
	switch(step)
	{
		case 0:
	
    dataLength = CF_SEG_NUM * (tCmdConfig.emComScan + 1);

    CF_SendCommandPackage(CmdCommand);
    CF_SendSetPackage();
    CF_SendCommandPackage(CmdData);

    checksum = 0;
 
			 break;
		case 1:
		for (i=0; i<16; i++)
		{
	    	CF_SendByte(CF_DisplayBuf[i]);
	  	  checksum +=CF_DisplayBuf[i];
		}

		break;
			case 2:
		for (i=16; i<32; i++)
		{
	    	CF_SendByte(CF_DisplayBuf[i]);
		    checksum +=CF_DisplayBuf[i];
		}

		break;
			case 3:
		for (i=32; i<48; i++)
		{
	  	 CF_SendByte(CF_DisplayBuf[i]);
	  	 checksum +=CF_DisplayBuf[i];
		}

		break;
			case 4:
		for (i=48; i<64; i++)
		{
		CF_SendByte(CF_DisplayBuf[i]);
		 checksum +=CF_DisplayBuf[i];
		}

		break;
			case 5:
		for (i=64; i<80; i++)
		{
		CF_SendByte(CF_DisplayBuf[i]);
		 checksum +=CF_DisplayBuf[i];
		}

		break;
			case 6:
		for (i=80; i<96; i++)
		{
		CF_SendByte(CF_DisplayBuf[i]);
		 checksum +=CF_DisplayBuf[i];
		}

		break;
			case 7:
		for (i=96; i<112; i++)
		{
		CF_SendByte(CF_DisplayBuf[i]);
		 checksum +=CF_DisplayBuf[i];
		}

		break;
			case 8:
		for (i=112; i<128; i++)
		{
		CF_SendByte(CF_DisplayBuf[i]);
		 checksum +=CF_DisplayBuf[i];
		}

		break;
			case 9:

		CF_SendByte(checksum);
	  CF_SendCommandPackage(CmdDataUpdate); 
    CF_SendSCK();

		break;
		default:
			break;
	}
}
/**
  * @function CF_DisplayIntoSleep()
  * ------------------------------
  * @brief    Led driver into sleep mode.
  * @param    None
  * @note     None
  */
void CF_DisplayIntoSleep(void)
{
    CF_SendCommandPackage(CmdCommand);
    tCmdConfig.emSleepFunction = SleepEnable;
    CF_SetControlCommand();
    CF_SendSetPackage();
    CF_SendCommandPackage(CmdSleep); 
    CF_SendSCK();
}

/**
  * @function CF_DisplayWakeUp()
  * ---------------------------
  * @brief    Led driver wakeup from sleep mode.
  * @param    None
  * @note     None
  */
void CF_DisplayWakeUp(void)
{
    CF_SendCommandPackage(CmdWakeUp);
    CF_SendCommandPackage(CmdCommand);
    tCmdConfig.emSleepFunction = SleepDisable;
    CF_SetControlCommand();
    CF_SendSetPackage();
    CF_SendSCK();
}

/**
  * @function CF_DisplayBreathBuf()
  * ------------------------------
  * @brief    Show breathing effect in display buffer.
  * @param    BREATH_TIME - time to run a single step
  * @retval   sbreathBuf  - breathing display buf
  */
uint8 CF_DisplayBreathBuf(void)
{
    static uint8 sbreathCount = 0;
    static uint8 sbreathBuf = 0;
    static uint8 sbreathFlag = 0;
   
    if (!sbreathFlag)
    {
        if ((sbreathCount + 1) < sizeof(BreathGamma))
            sbreathCount++;
        else
            sbreathFlag = 1;
    }
    else
    {
        if (sbreathCount > 0)
            sbreathCount--;
        else
            sbreathFlag = 0;
    }
    sbreathBuf = BreathGamma[sbreathCount];

    return (sbreathBuf);
}

/**
  * @function CF_DisplayMarqueeBuf()
  * -------------------------------
  * @brief    Show marquee effect in display buffer.
  * @param    MARQUEE_TIME - time to run a single step
  * @retval   smarqueeBuf  - address of marquee display buf
  */
uint8* CF_DisplayMarqueeBuf(void)
{
    uint8 i, tmp;
    static uint8 smarqueeBuf[MARQUEE_NUM] = {8, 16, 32, 64, 128, 255};

    //if (MARQUEE_TIME)
    {
        for (i=(MARQUEE_NUM-1); i>0; i--)
        {
            if (i == (MARQUEE_NUM-1))
                tmp = smarqueeBuf[i];
            smarqueeBuf[i] = smarqueeBuf[i-1];

            if (i == 1)
                smarqueeBuf[i-1] = tmp;
        }
    }

    return (smarqueeBuf);
}

/* [] END OF FILE */
