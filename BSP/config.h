/**
  ******************************************************************************
  * @file    config.h
  * @author 
  * @version V1.0
  * @date
  * @brief   System-wide configuration, includes, macros and shared data declarations.
  ******************************************************************************
  */
#ifndef		__CONFIG_H__
#define		__CONFIG_H__
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "stdint.h"
#include "stdio.h"
#include "stm32f030x8.h"
#include "gpio.h"
#include "adc.h"
#include "dma.h"
#include "usart.h"
//BSP
#include "buzz.h"
#include "key.h"
#include "temp.h"
#include "wireless.h"
#include "C8721.h"
#include "screen_c8721.h"
//COP
#include "main_control.h"
//#include "idle.h"
//#include "test.h"
#include "start_up.h"
//#include "run.h"
//#include "set.h"
#include "shutdown.h"
#include "warning.h"
//#include "ota.h"
//#include "pid.h"
//#include "gagent_md5.h"
//OS
#include "TaskScheduler.h"
/****************************************************************************************************************/
#define BITBAND(addr, bitnum) ((addr & 0xF0000000)+0x2000000+((addr &0xFFFFF)<<5)+(bitnum<<2)) 
#define MEM_ADDR(addr)  *((volatile unsigned long  *)(addr)) 
#define BIT_ADDR(addr, bitnum)   MEM_ADDR(BITBAND(addr, bitnum)) 
#define GPIOA_ODR_Addr    (GPIOA_BASE+12) //0x4001080C 
#define GPIOB_ODR_Addr    (GPIOB_BASE+12) //0x40010C0C 
#define GPIOC_ODR_Addr    (GPIOC_BASE+12) //0x4001100C 
#define GPIOD_ODR_Addr    (GPIOD_BASE+12) //0x4001140C 
#define GPIOE_ODR_Addr    (GPIOE_BASE+12) //0x4001180C 
#define GPIOF_ODR_Addr    (GPIOF_BASE+12) //0x40011A0C    
#define GPIOG_ODR_Addr    (GPIOG_BASE+12) //0x40011E0C    
#define GPIOA_IDR_Addr    (GPIOA_BASE+8) //0x40010808 
#define GPIOB_IDR_Addr    (GPIOB_BASE+8) //0x40010C08 
#define GPIOC_IDR_Addr    (GPIOC_BASE+8) //0x40011008 
#define GPIOD_IDR_Addr    (GPIOD_BASE+8) //0x40011408 
#define GPIOE_IDR_Addr    (GPIOE_BASE+8) //0x40011808 
#define GPIOF_IDR_Addr    (GPIOF_BASE+8) //0x40011A08 
#define GPIOG_IDR_Addr    (GPIOG_BASE+8) //0x40011E08 
#define PAout(n)   BIT_ADDR(GPIOA_ODR_Addr,n)
#define PAin(n)    BIT_ADDR(GPIOA_IDR_Addr,n)
#define PBout(n)   BIT_ADDR(GPIOB_ODR_Addr,n)
#define PBin(n)    BIT_ADDR(GPIOB_IDR_Addr,n)
#define PCout(n)   BIT_ADDR(GPIOC_ODR_Addr,n)
#define PCin(n)    BIT_ADDR(GPIOC_IDR_Addr,n)
#define PDout(n)   BIT_ADDR(GPIOD_ODR_Addr,n)
#define PDin(n)    BIT_ADDR(GPIOD_IDR_Addr,n)
#define PEout(n)   BIT_ADDR(GPIOE_ODR_Addr,n)
#define PEin(n)    BIT_ADDR(GPIOE_IDR_Addr,n)
#define PFout(n)   BIT_ADDR(GPIOF_ODR_Addr,n)
#define PFin(n)    BIT_ADDR(GPIOF_IDR_Addr,n)
#define PGout(n)   BIT_ADDR(GPIOG_ODR_Addr,n)
#define PGin(n)    BIT_ADDR(GPIOG_IDR_Addr,n)
#define delay_ms(X)    HAL_Delay(X);   
#ifndef MATHIS_FW_VERSION
#define MATHIS_FW_VERSION                100u
#endif
#define system_version                   MATHIS_FW_VERSION
#define unitF                            0
#define unitC                            1
#define off                              0
#define on                               1
#define ShutdownTime                     100
/****************************************************************************************************************/
//buzzer
#define buzzer_on_gpio     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8,GPIO_PIN_SET)
#define buzzer_off_gpio    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8,GPIO_PIN_RESET)
//key
#define key0            HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5)
#define key1            HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4)
//light
#define run_led_toggle  		HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_13)
//screen
#define  CS0   0
#define  CS1   1
//bluetooth
#define BT_On              HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11,GPIO_PIN_SET)
#define BT_Off             HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11,GPIO_PIN_RESET)
#define BT_Reset           HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12,GPIO_PIN_SET)
#define BT_State            HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_11)
//main_control
#define Power_On           HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,GPIO_PIN_SET)
#define Power_Off          HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,GPIO_PIN_RESET)

#define WarningTemp_ErH             1290
#define  HaveTempErr                999
typedef enum 
{
	idle,	
	test,
	start_up,
	run,
	set,
	shutdown,
	warning,
	ota
}_work_process;
extern _work_process work_process,work_process_backups;
typedef struct 
{
	uint8_t     units;     //0 F  1 C	     
	uint8_t     bluetooth;
  uint16_t    rtd_temp;
	uint16_t    proble0_temp;
	uint16_t    proble1_temp;
  uint16_t    icon;
  int16_t     pt1000_temp[9];  
  uint16_t    InTempSet;	  
}_system_data;   
extern _system_data system_data;

extern volatile uint8_t g_probe_connected;
extern volatile uint8_t g_probe_over_hi;
extern volatile uint8_t g_probe_over_lo;
extern volatile uint8_t g_battery_low;
extern volatile uint16_t g_battery_mv;
typedef struct {
    uint8_t RtdErr;
    uint8_t HighTempErr;
} ErrMessage;
extern ErrMessage SystemErrMessage;

#endif








