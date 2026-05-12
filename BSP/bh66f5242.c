#include "config.h"

/* Compatibility wrapper layer.
 * Real temperature acquisition is implemented in BSP/temp.c via USART2 DMA.
 */

uint8_t TempUnplugeErr[11] = {0};

//void Get_Filter_ADC12bitResult(void)
//{
//    /* Kept for compatibility with legacy calls; handled in TempGetTask(). */
//}

Temp_GetTypeDef ThermocoupleTempGet_Task(void)
{
    TempGetTask();
    return Temperature;
}

void TempGet_Init(void)
{
    Temp_Get_Init();
}

void TempInit(void)
{
    Temp_Get_Init();
}

void TempStabDis(void)
{
}

void LCDDisplayCalculation(Temp_GetTypeDef* t)
{
    (void)t;
}

uint32_t adc_filter(uint8_t channel)
{
    (void)channel;
    return 0u;
}
