/***************************用于定义各种接口******************************/
#ifndef		__MAIN_CONTROL_H__
#define		__MAIN_CONTROL_H__
#include "config.h"
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

