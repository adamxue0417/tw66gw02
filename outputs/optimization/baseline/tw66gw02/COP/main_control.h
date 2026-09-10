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

#define FIT_TEMP_C_THRESHOLD             (50)
#define FIT_OUTLIER_DIFF_C               (4)
#define FIT_MAIN_OUTLIER_DIFF_C          (4)
#define FIT_MAIN_OUTLIER_HOLD_COUNT      (2u)
#define FIT_MAIN_DYNAMIC_STEP_C          (1)
#define FIT_MAIN_DYNAMIC_CONFIRM_COUNT   (1u)
#define PROBE_REINSERT_MIN_OFF_TICKS_100MS (5u)
#define TEMP_COEFF_CHANNEL_COUNT          (3u)
#define TEMP_COEFF_TERM_COUNT             (5u)
#define MATHIS_MODEL_ID                    (0x04u)

enum {
    BT_OFF = 0,
    BT_PAIRING = 1,
    BT_CONNECTED = 2,
    BT_RECONNECTING = 3
};

typedef struct {
    int16_t temp_c[4];       /* Cavity, Left(O), Right(D), Probe */
    uint8_t valid_mask;      /* bits 0..3 correspond to temp_c[] */
    uint8_t high_mask;
    uint8_t low_mask;
    uint8_t units;
    uint8_t battery_percent;
    uint8_t error_code;
} MathisTelemetrySnapshot;

void MainControl(void);
void SysDataInit(void);
void UI_NotifyLocalInteraction(void);
void UI_CycleDisplayMode(void);
void UI_RequestBluetoothPairing(void);
void UI_RequestBluetoothPowerOff(void);
void UI_RequestBluetoothToggle(void);
void UI_SetBluetoothConnectionState(uint8_t connected);
uint8_t UI_GetBluetoothState(void);
int16_t UI_GetDisplayTemp(void);
uint8_t UI_GetDisplaySpecial(void);
uint8_t UI_GetBluetoothIconState(void);
uint8_t UI_IsProbeConnected(void);
int16_t UI_GetAdjustedMainTemp(uint8_t src_idx);
uint8_t UI_SetTempCoeff(uint8_t src_idx, const float coeff[TEMP_COEFF_TERM_COUNT]);
uint8_t UI_GetTempCoeff(uint8_t src_idx, float coeff[TEMP_COEFF_TERM_COUNT]);
uint8_t UI_SetUnits(uint8_t units, uint8_t persist);
void UI_FactoryReset(void);
void UI_GetTelemetrySnapshot(MathisTelemetrySnapshot *snapshot);
#endif

