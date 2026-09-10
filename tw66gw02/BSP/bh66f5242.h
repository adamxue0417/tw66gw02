#ifndef MATHIS_BH66F5242_H
#define MATHIS_BH66F5242_H
#include "temp.h"
/* Compatibility entry points use the canonical temperature types and constants. */
extern uint8_t TempUnplugeErr[11];
void TempGet_Init(void);
void TempInit(void);
void TempStabDis(void);
Temp_GetTypeDef ThermocoupleTempGet_Task(void);
void LCDDisplayCalculation(Temp_GetTypeDef *temperature);
uint32_t adc_filter(uint8_t channel);
#endif
