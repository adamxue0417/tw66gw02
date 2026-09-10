/**
  ******************************************************************************
  * @file    battery_level.c
  * @author 
  * @version V1.0
  * @date
  * @brief   Battery level lookup and power switching implementation.
  ******************************************************************************
  */
///******************************************************************************
// * @brief   Battery Level Monitoring Module Implementation
// * @details Implements battery voltage monitoring, percentage calculation, and
// *          power source management for 4-cell 18650 battery pack
// ******************************************************************************/
//#include "battery_level.h"
#include "config.h"

///**
// * @brief 18650 battery voltage to percentage lookup table
// * @details Single cell voltage values (in mV) for different charge levels
// *          Gets multiplied by battery_num (4) during initialization
// */
//u16 battery_voltage_table[13]={
// 4200,     /* 100% - Fully charged                          Index 0  */
// 4080,     /* 90%  - High charge                            Index 1  */
// 4000,     /* 80%  - Good charge                            Index 2  */
// 3930,     /* 70%  - 5-bar display                          Index 3  */
// 3870,     /* 60%  - Medium-high charge                     Index 4  */
// 3820,     /* 50%  - 4-bar display (half charge)            Index 5  */
// 3790,     /* 40%  - Medium-low charge                      Index 6  */
// 3770,     /* 30%  - Low charge                             Index 7  */
// 3730,     /* 20%  - 3-bar display (very low)               Index 8  */
// 3700,     /* 15%  - Critical low                           Index 9  */
// 3680,     /* 10%  - 2-bar display (shutdown warning)       Index 10 */
// 3500,     /*  5%  - Under-voltage warning                  Index 11 */
// 2500      /*  0%  - Under-voltage shutdown                 Index 12 */
//};

///**
// * @brief Initialize battery voltage table
// * @details Converts single-cell voltages to total pack voltage
// *          by multiplying each value by number of cells in series
// */
//void BatteryInit(void)
//{
//	u8 i;
//	/* Multiply each voltage level by number of cells (4) */
//	for(i=0;i<13;i++)
//	{
//		battery_voltage_table[i]=battery_voltage_table[i]*battery_num;
//	}
//}

///* Global Variables */
//u32 battery_voltage;           /* Calculated battery voltage */
//u32 TypeC_adc_value=0;         /* Type-C input voltage (in 0.1V units) */
//u16 Battery_Percentage=0;      /* Battery percentage (0-100%) */
//u32 VolTest=0;                 /* Voltage test/debug variable */

///**
// * @brief Get battery level and calculate percentage
// * @return Battery level (0-4, where 0=empty, 4=full)
// * @details Reads ADC, converts to voltage, and calculates percentage
// *          Uses linear interpolation for percentage calculation
// */
//u8 BatteryLevelGet(void)
//{
//	static u8  battery_level=5;         /* Battery level indicator (0-4) */
//	static u8  battery_level_init_flag=1; /* Initialization flag */
//	static u32 battery_adc_value=0;     /* Battery voltage ADC reading */

//	/* Read battery voltage from ADC channel 1 */
//	/* Formula: (ADC_value * 7 * 330) / 4096 = voltage in 0.1V units */
//	/* 7x voltage divider, 3.3V reference, 12-bit ADC */
//	battery_adc_value=7*330*adc_value[1]/4096;  /* Battery voltage (0.1V units) */

//	/* Read Type-C input voltage from ADC channel 0 */
//	TypeC_adc_value=7*330*adc_value[0]/4096;    /* Type-C voltage (0.1V units) */

//	VolTest=battery_adc_value;                  /* Copy for debugging */

//	/* Calculate battery percentage using linear interpolation */
//	/* Voltage range: 450 (0%) to 590 (100%) in 0.1V units (4.5V to 5.9V) */
//	if(Battery_Percentage>590)                  /* Above maximum voltage */
//	{
//		Battery_Percentage=100;                 /* Clamp to 100% */
//	}
//	else if(battery_adc_value<450)              /* Below minimum voltage */
//	{
//		Battery_Percentage=0;                   /* Clamp to 0% (battery dead) */
//	}
//	else                                        /* Normal voltage range */
//	{
//		/* Linear equation: y = 0.7143x - 321.43 */
//		/* Maps 450 (4.5V) to 0% and 590 (5.9V) to 100% */
//		Battery_Percentage=0.7143*battery_adc_value-321.43;
//	}

//	/* Original level-based implementation (commented out):
//	 * Divides voltage range into 5 discrete levels (0-4)
//	 * Each level represents a range of voltages
//	 */
////	if(battery_adc_value<450)                   /* < 4.5V */
////	{
////		if(battery_adc_value<400)               /* < 4.0V (critical) */
////		{
////			ErrCode=0x05;                       /* Set low battery error */
////		}
////		battery_level=0;                        /* Empty battery */
////	}
////	else if(battery_adc_value<=470)             /* 4.5V - 4.7V */
////	{
////		battery_level=1;                        /* 1 bar */
////	}
////	else if(battery_adc_value<=500)             /* 4.7V - 5.0V */
////	{
////		battery_level=2;                        /* 2 bars */
////	}
////	else if(battery_adc_value<=520)             /* 5.0V - 5.2V */
////	{
////		battery_level=3;                        /* 3 bars */
////	}
////	else                                        /* > 5.2V */
////	{
////		battery_level=4;                        /* 4 bars (full) */
////	}
////	return battery_level;
//}

///**
// * @brief Control power switching between battery and Type-C
// * @details Manages power source selection based on Type-C presence and battery voltage
// *          Switches to Type-C when available and battery is sufficient
// */
//void Power_Switch(void)
//{
//	/* Currently disabled */
//	/* Original implementation:
//	 * - Default: power switch disabled
//	 * - If Type-C voltage > 2.0V AND battery voltage > 4.5V:
//	 *   - Enable power switch (use Type-C power)
//	 * - Otherwise:
//	 *   - Disable power switch (use battery power)
//	 */
////	Power_Switch_disen;                         /* Default: disable switch */
////	if(TypeC_adc_value>200 && VolTest>450)      /* Type-C present & battery OK */
////	{
////		Power_Switch_en;                        /* Enable power switch */
////	}
////	else                                        /* Type-C absent or battery low */
////	{
////		Power_Switch_disen;                     /* Disable power switch */
////	}
//}
