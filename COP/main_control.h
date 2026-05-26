/**
  ******************************************************************************
  * @file    main_control.h
  * @author 
  * @version V1.0
  * @date
  * @brief   Main control state machine and UI runtime interface definitions.
  ******************************************************************************
  */
#ifndef		__MAIN_CONTROL_H__
#define		__MAIN_CONTROL_H__
#include "config.h"

#define IDLE_AUTO_SHUTDOWN_TICKS_100MS  (3000u)
#define FIT_TEMP_C_THRESHOLD             (50)
#define FIT_OUTLIER_DIFF_C               (4)
#define FIT_MAIN_OUTLIER_DIFF_C          (4)
#define FIT_MAIN_OUTLIER_HOLD_COUNT      (2u)
#define FIT_MAIN_DYNAMIC_STEP_C          (1)
#define FIT_MAIN_DYNAMIC_CONFIRM_COUNT   (1u)
#define PROBE_REINSERT_MIN_OFF_TICKS_100MS (5u)



void MainControl(void);
void SysDataInit(void);
void UI_NotifyLocalInteraction(void);
void UI_CycleDisplayMode(void);
void UI_RequestBluetoothPairing(void);
void UI_RequestBluetoothPowerOff(void);
void UI_SetBluetoothConnectionState(uint8_t connected);
int16_t UI_GetDisplayTemp(void);
uint8_t UI_GetDisplaySpecial(void);
uint8_t UI_GetBluetoothIconState(void);
uint8_t UI_IsProbeConnected(void);
#endif

