/**
  ******************************************************************************
  * @file    temp.h
  * @author 
  * @version V1.0
  * @date
  * @brief   Temperature and battery acquisition interface definitions.
  ******************************************************************************
  */
#ifndef   __TEMP_H__
#define   __TEMP_H__
#include "config.h"
#define TempCount 4
#define USR_ADC_COUNT	4
#define USR_ADC_CH_NR	2
#define  TempErr 999
#define  TempHigh 998
#define  TempLow 997
#define  TempDisconnected (-30)
typedef struct channel_evn_s {
	uint16_t max;
	uint16_t min;
	uint16_t vol;
	uint32_t sum;
} channel_env_t;

typedef struct
{			
	int	InTtempDis;	
	int InTtempCalDis;
	uint8_t TemperatureUnits;
	uint16_t	Proble1;	
	uint16_t	Proble2;
	uint16_t	Proble3;	
	uint16_t	ProbleC;		
	uint16_t	InTtemp;	
	uint16_t InTempMaxValue;
	uint16_t InTempMinValue;
	uint16_t ProbeMaxValue;
	uint16_t ProbeMinValue;
	uint16_t	InTtempSaveHigh;	
	uint16_t	InTtempOriginal;	
} Temp_GetTypeDef;
extern Temp_GetTypeDef Temperature;
extern volatile uint8_t ADC_Start_DMA_OVER;
extern channel_env_t ch_env[USR_ADC_CH_NR];
void TempGetTask(void);
void Temp_Get_Init(void);
void TempGetTask(void);
#endif

