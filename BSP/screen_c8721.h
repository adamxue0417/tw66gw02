/**
  ******************************************************************************
  * @file    screen_c8721.h
  * @author 
  * @version V1.0
  * @date
  * @brief   C8721 screen mapping and display interface definitions.
  ******************************************************************************
  */
#ifndef __SCREEN_C8721_H__
#define __SCREEN_C8721_H__

#include "stdint.h"
#include "string.h"
#include "stm32f030x8.h"
#include "stm32f0xx_hal.h"
#include "gpio.h"

/* C8721 SCL = PB13, SDA = PB14 */
#define C8721_SCL_H()   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET)
#define C8721_SCL_L()   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET)
#define C8721_SDA_H()   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET)
#define C8721_SDA_L()   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET)

/* 9 segments (TempA .. TempI), each 2-bit wide.
 * Segment value: GAUGE_OFF = 0, GAUGE_ON = 3 (both bits set).
 */
#define GAUGE_SEG_COUNT     9u
#define GAUGE_SEG_ON        3u   /* 2-bit value: fully on  */
#define GAUGE_SEG_OFF       0u   /* 2-bit value: fully off */


/*
 * Display data union.
 *
 * DisplayMapTable[0..15] maps 1:1 to the C8721 LED matrix:
 *   bit s of DisplayMapTable[g]  ->  CF_DisplayBuf[g + s * 16]
 *   (bit=1 -> 0xFF full brightness, bit=0 -> 0x00 off)
 *
 * Bytes 0-8 hold the packed bit-field segments/icons.
 * Bytes 9-15 are available for additional segment data.
 * Bytes 16-39 hold wider state fields shared with upper-layer logic.
 */
typedef union
{
    uint8_t DisplayMapTable[40];
    struct
    {
        /* --- LED segment / icon region (bytes 0-8) --- */
        uint8_t SearHundred0    :7;
        uint8_t BlutoothIcon    :1;  /* byte 0 */
        uint8_t SearHundred1    :7;
        uint8_t PIcon           :1;  /* byte 1 */
        uint8_t SearTen0        :7;
        uint8_t UnitCIcon       :1;  /* byte 2 */
        uint8_t SearTen1        :7;
        uint8_t UnitFIcon       :1;  /* byte 3 */
        uint8_t SearLow0        :7;
        uint8_t OIcon           :1;  /* byte 4 */
        uint8_t SearLow1        :7;
        uint8_t DIcon           :1;  /* byte 5 */
        uint8_t TempA           :2;
        uint8_t TempB           :2;
        uint8_t TempC           :2;
        uint8_t BatteryLowIcon  :1;
        uint8_t CavityIcon1     :1;  /* byte 6 */
        uint8_t TempD           :2;
        uint8_t TempE           :2;
        uint8_t TempF           :2;
        uint8_t CavityIcon2     :2;  /* byte 7 */
        uint8_t TempG           :2;
        uint8_t TempH           :2;
        uint8_t TempI           :2;
        uint8_t CavityIcon3     :2;  /* byte 8 */

        /* --- Upper-layer state fields (byte 16 onwards in the union) --- */
        uint16_t act_temp[9];
        uint8_t  states_wifi;
        uint8_t  icon_heat        :1;
        uint8_t  icon_mot         :1;
        uint8_t  icon_fan         :1;
        uint8_t  icon_light       :1;
        uint8_t  icon_logo        :1;
        uint8_t  icon_alarm       :1;
        uint8_t  icon_uints       :1;
        uint8_t  states_bluetooth :1;
        uint8_t  states_pellet    :2;
        uint8_t  r1               :6;
        uint32_t timer_alarm_seconds;
        uint16_t screen_idex;
        uint16_t screen_select_idex;
        uint16_t set_temp[9];
        uint16_t control_mode;
        uint16_t mamal_control;
        uint16_t touch_idex;
    } DisplayMap;
} _screen_data;

extern _screen_data Screen_Data;

typedef enum {
    DISPLAY_MODE_D_SURFACE,   // 0
    DISPLAY_MODE_O_SURFACE,   // 1
    DISPLAY_MODE_PROBE,       // 2
    DISPLAY_MODE_CAVITY       // 3
} DisplayMode_t;


extern DisplayMode_t DisplayMode;




/*
 * Low-level GPIO port functions called by C8721 driver via:
 *   #define CF_SCL_Write(x)  SCL_Write(x)
 *   #define CF_SDA_Write(x)  SDA_Write(x)
 * Implemented in screen_c8721.c using STM32 HAL.
 */
void SCL_Write(uint8_t x);
void SDA_Write(uint8_t x);

void Screen_C8721_Init(void);
void DisplayTask(void);
void Screen_C8721_PrepareShutdown(void);

#endif /* __SCREEN_C8721_H__ */


