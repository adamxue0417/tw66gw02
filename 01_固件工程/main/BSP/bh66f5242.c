/**
  ******************************************************************************
  * @file    bh66f5242.c
  * @author 
  * @version V1.0
  * @date
  * @brief   历史测温兼容接口；实际采集在 BSP/temp.c。
  ******************************************************************************
  */
#include "config.h"

/* Compatibility wrapper layer.
 * Real temperature acquisition is implemented in BSP/temp.c via USART2 DMA.
 */

uint8_t TempUnplugeErr[11] = {0};

//void Get_Filter_ADC12bitResult(void)
//{
//    /* Kept for compatibility with legacy calls; handled in TempGetTask(). */
//}
/**
  * @function ThermocoupleTempGet_Task()
  * --------------------------
  * @brief    Execute thermocouple temperature acquisition task.
  * @param    None
  * @note     None
  */
Temp_GetTypeDef ThermocoupleTempGet_Task(void)
{
    TempGetTask();
    return Temperature;
}
/**
  * @function TempGet_Init()
  * --------------
  * @brief    Initialize temperature acquisition resources.
  * @param    None
  * @note     None
  */
void TempGet_Init(void)
{
    Temp_Get_Init();
}
/**
  * @function TempInit()
  * ------------
  * @brief    Initialize temperature module state.
  * @param    None
  * @note     None
  */
void TempInit(void)
{
    Temp_Get_Init();
}
/**
  * @function TempStabDis()
  * -------------
  * @brief    兼容空接口；当前不处理显示稳定。
  * @param    None
  * @note     None
  */
void TempStabDis(void)
{
}
/**
  * @function LCDDisplayCalculation()
  * -----------------------
  * @brief    兼容空接口；忽略输入，不计算显示值。
  * @param    t - input parameter
  * @note     None
  */
void LCDDisplayCalculation(Temp_GetTypeDef* t)
{
    (void)t;
}
/**
  * @function adc_filter()
  * ------------
  * @brief    兼容占位接口；当前固定返回 0，不执行滤波。
  * @param    channel - input parameter
  * @note     None
  */
uint32_t adc_filter(uint8_t channel)
{
    (void)channel;
    return 0u;
}
