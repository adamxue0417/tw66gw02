/******************************************************************************
 * @file    bh66f5242.h
 * @brief   BH66F5242 Temperature Acquisition Module Header File
 * @details This module handles temperature reading from RTD (PT1000) sensors
 *          and thermocouples via UART communication with BH66F5242 chips,
 *          includes ADC filtering, calibration, and LCD display calculation
 ******************************************************************************/
#ifndef		__BH66F5242_H__
#define		__BH66F5242_H__
#include "config.h"

/* Temperature Error Definitions */
#define  HaveTempErr    1          /* Temperature error present flag */
#define  NoTempErr      0          /* No temperature error flag */

/* ADC Configuration */
#define USR_ADC_COUNT	  10       /* Number of ADC conversions to average */
#define USR_ADC_CH_NR	  6        /* Number of ADC channels */

/* Error Temperature Value */
#define TempErr         999        /* Error temperature indicator value */

/* External Variables */
extern u8 TempUnplugeErr[11];      /* Temperature sensor error flags array */

/**
 * @brief Temperature readings structure
 * @details Contains all temperature sensor readings and calibrated values
 */
typedef struct
{
	int	InTemp;	                   /* Internal temperature (RTD) in Fahrenheit */
	int InTtempCalDis;             /* Calibrated display temperature in Fahrenheit */
	int Thermocouple1;             /* Thermocouple 1 reading in Fahrenheit */
	int Thermocouple2;             /* Thermocouple 2 reading in Fahrenheit */
	int Thermocouple3;             /* Thermocouple 3 reading in Fahrenheit */
	u16 IntTempOriginal;           /* Original internal temperature value */
} Temp_GetTypeDef;

extern Temp_GetTypeDef Temperature;  /* Global temperature structure */

/**
 * @brief ADC channel environment structure
 * @details Stores min, max, average and sum for each ADC channel
 */
typedef struct channel_evn_s {
	u16 max;                       /* Maximum ADC value in current batch */
	u16 min;                       /* Minimum ADC value in current batch */
	u16 vol;                       /* Averaged ADC value (after filtering) */
	u32 sum;                       /* Sum of ADC values for averaging */
} channel_env_t;

extern channel_env_t ch_env[USR_ADC_CH_NR];  /* ADC channel data array */
extern u8 ADC_Start_DMA_OVER;                /* ADC DMA completion flag */

/**
 * @brief Initialize temperature acquisition module
 */
void TempGet_Init(void);

/**
 * @brief Configure ADC peripheral
 */
void adc_config(void);

/**
 * @brief Get filtered 12-bit ADC results
 * @details Triggers UART communication with temperature acquisition chips
 */
void Get_Filter_ADC12bitResult(void);

/**
 * @brief Initialize temperature system
 */
void TempInit(void);

/**
 * @brief Stabilize temperature display
 * @details Filters rapid temperature changes for stable display
 */
void TempStabDis(void);

/**
 * @brief Main temperature acquisition task
 * @return Temp_GetTypeDef: Structure with all temperature readings
 */
Temp_GetTypeDef ThermocoupleTempGet_Task(void);

/**
 * @brief Calculate LCD display temperature with smoothing
 * @param Temperature: Pointer to temperature structure
 * @details Implements gradual temperature change algorithm for stable display
 */
void LCDDisplayCalculation(Temp_GetTypeDef* Temperature);

/**
 * @brief Apply moving average filter to ADC channel
 * @param channel: ADC channel number (0-7)
 * @return Filtered ADC value
 */
u32 adc_filter(u8 channel);

#endif
