/**
  ******************************************************************************
  * @file    battery_level.h
  * @author 
  * @version V1.0
  * @date
  * @brief   Battery level monitoring interface definitions.
  ******************************************************************************
  */
#ifndef		__BATTERY_LEVEL_H__
#define		__BATTERY_LEVEL_H__
#include "config.h"

/* Battery Configuration */
#define battery_num      4     /* Number of battery cells in series (4 cells) */

/* External Variables */
extern u32 TypeC_adc_value;    /* Type-C charging voltage ADC value (in 0.1V units) */
extern u16 Battery_Percentage; /* Battery percentage (0-100%) */

/**
 * @brief Initialize battery voltage table
 * @details Multiplies voltage table values by number of cells to get total pack voltage
 */
void BatteryInit(void);

/**
 * @brief Get battery level indicator
 * @return Battery level (0-4, where 0=empty, 4=full)
 */
u8 BatteryLevelGet(void);

/**
 * @brief Control power switching based on battery and USB-C status
 * @details Manages power source selection between battery and USB-C input
 */
void Power_Switch(void);

#endif
