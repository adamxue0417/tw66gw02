/**
  ******************************************************************************
  * @file    C8721.h
  * @author 
  * @version V1.0
  * @date
  * @brief   C8721 command, display buffer and driver interface definitions.
  ******************************************************************************
  */
/* Define to prevent recursive inclusion --------------------------------------*/
#ifndef __C8721_H__
#define __C8721_H__

/* Includes -------------------------------------------------------------------*/
// #include "project.h"							   // include corresponding 'h' file

/* Exported typedef -----------------------------------------------------------*/
typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned long   uint32;
typedef signed   char   int8;
typedef signed   short  int16;
typedef signed   long   int32;
typedef          float  float32;

/* Exported define ------------------------------------------------------------*/
#define CF_BUF_SIZE         128
#define CF_SEG_NUM          16
//#define CF_TimeDlyUs()       {__NOP;   
// pin define
#define CF_SCL_Write(x)        SCL_Write(x)
#define CF_SDA_Write(x)        SDA_Write(x)

#ifdef TRUE
    #undef TRUE
#endif
#define TRUE                    (1u)

#ifdef FALSE
    #undef FALSE
#endif
#define FALSE                   (0u)

#define CF_LUMI_FULL            0xFFu
#define CF_LUMI_ACTIVE          0xD0u
#define CF_LUMI_HALF            0x7Fu
#define CF_LUMI_OFF             0u

#define MARQUEE_NUM             19

/* Command type define --------------------------------------------------------*/
/*
 * @brief  command type
 * @param  command type -> low 4 bits
 */
typedef enum em_cmd_type {
    CmdCommand     = 0x01,
    CmdData        = 0x02, 
    CmdDataUpdate  = 0x04,
    CmdSoftReset   = 0x08,
    CmdSleep       = 0x0E,
    CmdWakeUp      = 0x0D,
    CmdGlobalReset = 0x0B,
    CmdDataBack    = 0x07
} em_cmd_type_t;

/*
 * @brief  current gain
 * @param  0~63 -> 8.5~40mA
 * @note   Iout = 0.5 * (17 + G)
 */
typedef enum em_cmd_current_gain {
    mA_8_5    = 0,
    mA_9      = 1,
    mA_9_5    = 2,
    mA_10     = 3,
    mA_10_5   = 4,
    mA_11     = 5,
    mA_11_5   = 6,
    mA_12     = 7,
    mA_12_5   = 8,
    mA_13     = 9,
    mA_13_5   = 10,
    mA_14     = 11,
    mA_14_5   = 12,
    mA_15     = 13,
    mA_15_5   = 14,
    mA_16     = 15,
    mA_16_5   = 16,
    mA_17     = 17,
    mA_17_5   = 18,
    mA_18     = 19,
    mA_18_5   = 20,
    mA_19     = 21,
    mA_19_5   = 22,
    mA_20     = 23,
    mA_20_5   = 24,
    mA_21     = 25,
    mA_21_5   = 26,
    mA_22     = 27,
    mA_22_5   = 28,
    mA_23     = 29,
    mA_23_5   = 30,
    mA_24     = 31,
    mA_24_5   = 32,
    mA_25     = 33,
    mA_25_5   = 34,
    mA_26     = 35,
    mA_26_5   = 36,
    mA_27     = 37,
    mA_27_5   = 38,
    mA_28     = 39,
    mA_28_5   = 40,
    mA_29     = 41,
    mA_29_5   = 42,
    mA_30     = 43,
    mA_30_5   = 44,
    mA_31     = 45,
    mA_31_5   = 46,
    mA_32     = 47,
    mA_32_5   = 48,
    mA_33     = 49,
    mA_33_5   = 50,
    mA_34     = 51,
    mA_34_5   = 52,
    mA_35     = 53,
    mA_35_5   = 54,
    mA_36     = 55,
    mA_36_5   = 56,
    mA_37     = 57,
    mA_37_5   = 58,
    mA_38     = 59,
    mA_38_5   = 60,
    mA_39     = 61,
    mA_39_5   = 62,
    mA_40     = 63
} em_cmd_current_gain_t;


/* Control command define -----------------------------------------------------*/
/*
 * @brief  clock mode command
 * @param  0 -> internal clock
 *         1 -> external clock
 */
typedef enum em_cmd_clock_mode {
    InternalClock = 0u,
    ExternalClock = 1u
} em_cmd_clock_mode_t;

/*
 * @brief  com scan command
 * @param  0x00~0x0F -> com1~com16
 */
typedef enum em_cmd_com_scan {
    Com1  = 0u,
    Com2  = 1u,
    Com3  = 2u,
    Com4  = 3u,
    Com5  = 4u,
    Com6  = 5u,
    Com7  = 6u,
    Com8  = 7u,
    Com9  = 8u,
    Com10 = 9u,
    Com11 = 10u,
    Com12 = 11u,
    Com13 = 12u,
    Com14 = 13u,
    Com15 = 14u,
    Com16 = 15u
} em_cmd_com_scan_t;

/*
 * @brief  work mode command
 * @param  0 -> normal work mode
 *         1 -> test work mode
 */
typedef enum em_cmd_work_mode {
    NormalMode = 0u,
    TestMode   = 1u
} em_cmd_work_mode_t;

/*
 * @brief  data update command
 * @param  0 -> data arbitrary update
 *         1 -> data automatic update
 */
typedef enum em_cmd_data_update {
    ArbitraryUpdate = 0u,
    AutomaticUpdate = 1u
} em_cmd_data_update_t;

/* Display command define -----------------------------------------------------*/
/*
 * @brief  125C over-temperature protection
 * @param  0 -> enable 125C OTP
 *         1 -> disable 125C OTP
 */
typedef enum em_cmd_over_t125_protect {
    Otp125Enable  = 0u,
    Otp125Disable = 1u
} em_cmd_over_t125_protect_t;

/*
 * @brief  set PWM clock
 * @param  0x00 -> 1M
 *         0x01 -> 2M
 *         0x02 -> 4M
 *         0x03 -> 8M
 */
typedef enum em_cmd_pwm_clock {
    Clock1M = 0u,
    Clock2M = 1u,
    Clock4M = 2u,
    Clock8M = 3u
} em_cmd_pwm_clock_t;

/*
 * @brief  display enable
 * @param  0 -> disable
 *         1 -> enable
 */
typedef enum em_cmd_display_enable {
    DisplayDisable = 0u,
    DisplayEnable  = 1u
} em_cmd_display_enable_t;

/*
 * @brief  eliminate the ghosting
 * @param  0 -> slight
 *         1 -> strong
 */
typedef enum em_cmd_ghost_reduction {
    Slight = 0u,
    Strong = 1u
} em_cmd_ghost_reduction_t;

/*
 * @brief  row break time
 * @param  0x00 -> 4 pwm cycle
 *         0x01 -> 8 pwm cycle
 *         0x02 -> 12 pwm cycle
 *         0x03 -> 16 pwm cycle
 */
typedef enum em_cmd_row_break_time {
    PwmCycle4  = 0u,
    PwmCycle8  = 1u,
    PwmCycle12 = 2u,
    PwmCycle16 = 3u
} em_cmd_row_break_time_t;

/* System command define ------------------------------------------------------*/
/*
 * @brief  sleep function
 * @param  0 -> disable
 *         1 -> enable
 */
typedef enum em_cmd_sleep_function {
    SleepDisable = 0u,
    SleepEnable  = 1u
} em_cmd_sleep_function_t;

/*
 * @brief  auto sleep function
 * @param  0 -> disable
 *         1 -> enable
 */
typedef enum em_cmd_auto_sleep_function {
    AutoSleepDisable = 0u,
    AutoSleepEnable  = 1u
} em_cmd_auto_sleep_function_t;

/*
 * @brief  global reset function
 * @param  0 -> disable
 *         1 -> enable
 */
typedef enum em_cmd_global_reset {
    GlobalDisable = 0u,
    GlobalEnable  = 1u
} em_cmd_global_reset_t;

/*
 * @brief  soft reset function
 * @param  0 -> disable
 *         1 -> enable
 */
typedef enum em_cmd_soft_reset {
    SoftDisable = 0u,
    SoftEnable  = 1u
} em_cmd_soft_reset_t;

/*
 * @brief  150C over-temperature protection
 * @param  0 -> enable 150C OTP
 *         1 -> disable 150C OTP
 */
typedef enum em_cmd_over_t150_protect {
    Otp150Enable  = 0u,
    Otp150Disable = 1u
} em_cmd_over_t150_protect_t;

/*
 * @brief  power down reset function
 * @param  0 -> disable
 *         1 -> enable
 */
typedef enum  
{
    PowerDisable = 0u,
    PowerEnable  = 1u
} em_cmd_power_down_reset_t;

/* Private variables ----------------------------------------------------------*/
typedef struct ty_cmd_config {
    em_cmd_type_t                   emCmdType;
    em_cmd_com_scan_t               emComScan;
    em_cmd_data_update_t            emDataUpdata;
    em_cmd_work_mode_t              emWorkMode;
    em_cmd_clock_mode_t             emClockMode;
    em_cmd_pwm_clock_t              emPwmClock;
    em_cmd_display_enable_t         emDisplay;
    em_cmd_ghost_reduction_t        emGhostReduction;
    em_cmd_row_break_time_t         emRowBreakTime;
    em_cmd_over_t125_protect_t      emOverT125Protect;
    em_cmd_current_gain_t           emCurrentGain;
    em_cmd_sleep_function_t         emSleepFunction;
    em_cmd_global_reset_t           emGlobalReset;
    em_cmd_soft_reset_t             emSoftReset;
    em_cmd_over_t150_protect_t      emOverT150Protect;
    em_cmd_auto_sleep_function_t    emAutoSleepFunction;
    em_cmd_power_down_reset_t       emPowerDownReset;
} ty_cmd_config_t;

typedef struct ty_cmd_setting {
    uint8 Current;
    uint8 Mode;
    uint8 Display;
    uint8 Control;
} ty_cmd_setting_t;

/* Exported variables ---------------------------------------------------------*/
/* display variable */
extern uint8 CF_DisplayBuf[CF_BUF_SIZE];
/* config variable */
extern ty_cmd_config_t tCmdConfig;
extern ty_cmd_setting_t tCmdSetting;

/* Exported functions ---------------------------------------------------------*/
extern void CF_Init(void);
extern void CF_DisplayClearBuf(void);
extern void CF_DisplayAll(uint8 lumi); 
extern void CF_DisplayBufAutomatic(void);
extern void CF_DisplayIntoSleep(void);
extern void CF_DisplayWakeUp(void);
extern uint8 CF_DisplayBreathBuf(void);
extern uint8* CF_DisplayMarqueeBuf(void);
extern void  CF_DisplaySegment(uint8 step);
#endif  /* __C8725_H__ */
