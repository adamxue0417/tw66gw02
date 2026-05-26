/**
  ******************************************************************************
  * @file    main_control.c
  * @author 
  * @version V1.0
  * @date
  * @brief   Main control state machine and UI runtime cache implementation.
  ******************************************************************************
  */
#include "config.h"

_work_process work_process,work_process_backups;
_system_data  system_data;

volatile uint8_t g_probe_connected = 0u;
volatile uint8_t g_probe_over_hi = 0u;
volatile uint8_t g_probe_over_lo = 0u;

enum {
    BT_OFF = 0,
    BT_PAIRING = 1,
    BT_CONNECTED = 2,
    BT_RECONNECTING = 3
};



static uint8_t s_bt_state = BT_OFF;
static uint8_t s_bt_icon_on = 0u;
static uint8_t s_bt_pairing_req = 0u;
static uint8_t s_bt_poweroff_req = 0u;
static uint8_t s_bt_connected_input = 0u;
static uint8_t s_bt_hw_on = 0u;
static uint16_t s_bt_blink_ticks_100ms = 0u;
static uint16_t s_bt_pairing_ticks_100ms = 0u;

static int16_t s_display_temp = 0;
/* 0:none, 1:-HI, 2:-LO, 3:--- */
static uint8_t s_display_special = 0u;

static uint16_t s_idle_ticks_100ms = 0u;
static _work_process s_last_work_process = idle;
static uint8_t s_probe_connected_last = 0u;
static uint8_t s_probe_absent_ticks_100ms = 0u;
static uint8_t s_main_outlier_hold_cnt[3] = {0u, 0u, 0u};
static uint8_t s_main_outlier_dynamic_cnt[3] = {0u, 0u, 0u};
static int16_t s_main_prev_src_c[3] = {0, 0, 0};
static uint8_t s_main_prev_src_valid[3] = {0u, 0u, 0u};
static int16_t s_fit_prev_c[4] = {0, 0, 0, 0};
static int16_t s_fit_trend_acc_c[4] = {0, 0, 0, 0};
static uint8_t s_fit_prev_valid[4] = {0u, 0u, 0u, 0u};

static void BluetoothHwPowerOn(void);
static void BluetoothHwPowerOff(void);

/**
  * @function IsMainTempInvalid()
  * -------------------
  * @brief    Check whether a main temperature sample is invalid.
  * @param    t - input parameter
  * @note     None
  */
static uint8_t IsMainTempInvalid(int16_t t)
{
    if ((t == HaveTempErr) || (t == TempDisconnected) || (t == TempHigh) || (t == TempLow)) {
        return 1u;
    }
    return 0u;
}

static uint8_t IsMainChannelDisconnected(int16_t t)
{
    if ((t == HaveTempErr) || (t == TempDisconnected)) {
        return 1u;
    }
    return 0u;
}

static uint8_t ApplyTempSpecial(int16_t t)
{
    if (t == TempHigh) {
        s_display_special = 1u;
        s_display_temp = 0;
        return 1u;
    }
    if (t == TempLow) {
        s_display_special = 2u;
        s_display_temp = 0;
        return 1u;
    }
    if ((t == HaveTempErr) || (t == TempDisconnected)) {
        s_display_special = 3u;
        s_display_temp = 0;
        return 1u;
    }
    return 0u;
}

/**
  * @function TempToC()
  * ------------
  * @brief    Convert temperature from current UI unit to Celsius.
  * @param    t - input parameter
  * @note     None
  */
static int16_t TempToC(int16_t t)
{
    if (system_data.units == unitC) {
        return t;
    }
    return (int16_t)((((int32_t)t - 32) * 5) / 9);
}

/**
  * @function TempFromC()
  * ------------
  * @brief    Convert temperature from Celsius to current UI unit.
  * @param    c - input parameter
  * @note     None
  */
static int16_t TempFromC(int16_t c)
{
    if (system_data.units == unitC) {
        return c;
    }
    return (int16_t)(((int32_t)c * 9) / 5 + 32);
}


/**
  * @function CalcMainConsensusTemp()
  * -----------------------
  * @brief    Apply main-channel consensus correction for outlier suppression.
  * @param    src_idx - input parameter
  * @param    raw_temp - input parameter
  * @param    out_display_temp - input parameter
  * @note     None
  */
static uint8_t CalcMainConsensusTemp(uint8_t src_idx, int16_t raw_temp, int16_t *out_display_temp)
{
    // Store valid main-channel temperatures (converted to Celsius) for consensus calculation.
    int16_t t[3];
    // Hold consensus reference temperature derived from valid channels.
    int16_t ref;
    // Hold current source temperature converted to Celsius.
    int16_t src_c;
    // Hold absolute difference between source and reference.
    int16_t d;
    // Count how many main channels are currently valid.
    uint8_t vcnt = 0u;

    // Reject invalid output pointer or unsupported source channel index.
    if ((out_display_temp == 0) || (src_idx > 2u)) {
        return 0u;
    }

    // Collect valid main-channel temperatures for robust consensus.
    if (IsMainTempInvalid(system_data.pt1000_temp[0]) == 0u) { t[vcnt++] = TempToC(system_data.pt1000_temp[0]); }
    if (IsMainTempInvalid(system_data.pt1000_temp[1]) == 0u) { t[vcnt++] = TempToC(system_data.pt1000_temp[1]); }
    if (IsMainTempInvalid(system_data.pt1000_temp[2]) == 0u) { t[vcnt++] = TempToC(system_data.pt1000_temp[2]); }

    // Need at least two valid channels to build a meaningful consensus.
    if (vcnt < 2u) {
        return 0u;
    }

    // With two channels use mean value; with three channels use median for outlier robustness.
    if (vcnt == 2u) {
        ref = (int16_t)(((int32_t)t[0] + (int32_t)t[1]) / 2);
    } else {
        // Sort three values and take middle one as median.
        if (t[0] > t[1]) { int16_t x = t[0]; t[0] = t[1]; t[1] = x; }
        if (t[1] > t[2]) { int16_t x = t[1]; t[1] = t[2]; t[2] = x; }
        if (t[0] > t[1]) { int16_t x = t[0]; t[0] = t[1]; t[1] = x; }
        ref = t[1];
    }

    // Convert source reading to Celsius so all thresholds stay unit-consistent.
    src_c = TempToC(raw_temp);
    if (src_c >= FIT_TEMP_C_THRESHOLD) {
        *out_display_temp = raw_temp;
        return 1u;
    }
    // Track dynamic behavior of the source channel to distinguish real movement from static drift.
    if (s_main_prev_src_valid[src_idx] != 0u) {
        int16_t step = (src_c >= s_main_prev_src_c[src_idx]) ? (src_c - s_main_prev_src_c[src_idx]) : (int16_t)(s_main_prev_src_c[src_idx] - src_c);
        // Count dynamic steps only when per-cycle movement reaches configured minimum.
        if (step >= FIT_MAIN_DYNAMIC_STEP_C) {
            if (s_main_outlier_dynamic_cnt[src_idx] < 255u) {
                s_main_outlier_dynamic_cnt[src_idx]++;
            }
        } else {
            // Reset dynamic counter when movement is too small.
            s_main_outlier_dynamic_cnt[src_idx] = 0u;
        }
    } else {
        // No previous sample means no dynamic evidence yet.
        s_main_outlier_dynamic_cnt[src_idx] = 0u;
    }
    // Save current source sample for next dynamic-step comparison.
    s_main_prev_src_c[src_idx] = src_c;
    s_main_prev_src_valid[src_idx] = 1u;

    // Measure source deviation from consensus reference.
    d = (src_c >= ref) ? (src_c - ref) : (int16_t)(ref - src_c);
    // Enter outlier handling only when deviation exceeds configured threshold.
    if (d > FIT_MAIN_OUTLIER_DIFF_C) {
        *out_display_temp = raw_temp;
        return 1u;
    }
    // Source returned to consensus range, so clear outlier tracking state.
    s_main_outlier_hold_cnt[src_idx] = 0u;
    s_main_outlier_dynamic_cnt[src_idx] = 0u;
    return 0u;
}

/**
  * @function CalcFittedTemp()
  * ----------------
  * @brief    Calculate fitted display temperature using probe and main channels.
  * @param    src_idx - input parameter
  * @param    raw_temp - input parameter
  * @param    out_display_temp - input parameter
  * @note     None
  */
static uint8_t CalcFittedTemp(uint8_t src_idx, int16_t raw_temp, int16_t *out_display_temp)
{
    // Probe temperature in Celsius used as fusion anchor.
    int16_t probe;
    // Main-channel temperatures converted to Celsius for distance checks.
    int16_t t0, t1, t2;
    // Absolute distance between a main channel and probe.
    int16_t d;
    // Weighted sum accumulator for fused output.
    int32_t wsum;
    // Total weight accumulator for fused output.
    int32_t w;
    int16_t src_c;
    int16_t src_step = 0;
    uint8_t trend_single = 0u;
    uint8_t trend_peer_ok = 0u;

    if (out_display_temp == 0) {
        return 0u;
    }
    if (src_idx > 3u) {
        return 0u;
    }
    src_c = TempToC(raw_temp);
    if (src_c >= FIT_TEMP_C_THRESHOLD) {
        *out_display_temp = raw_temp;
        return 1u;
    }

    /* If one channel keeps rising/falling by 4C, prefer standalone display for that channel.
     * Exception: if peers move in same direction and remain within original trigger diff,
     * keep this channel in fitting. */
    if (s_fit_prev_valid[src_idx] != 0u) {
        src_step = (int16_t)(src_c - s_fit_prev_c[src_idx]);
        if (src_step > 0) {
            if (s_fit_trend_acc_c[src_idx] >= 0) { s_fit_trend_acc_c[src_idx] = (int16_t)(s_fit_trend_acc_c[src_idx] + src_step); }
            else { s_fit_trend_acc_c[src_idx] = src_step; }
        } else if (src_step < 0) {
            if (s_fit_trend_acc_c[src_idx] <= 0) { s_fit_trend_acc_c[src_idx] = (int16_t)(s_fit_trend_acc_c[src_idx] + src_step); }
            else { s_fit_trend_acc_c[src_idx] = src_step; }
        } else {
            s_fit_trend_acc_c[src_idx] = 0;
        }
    } else {
        s_fit_trend_acc_c[src_idx] = 0;
    }
    s_fit_prev_c[src_idx] = src_c;
    s_fit_prev_valid[src_idx] = 1u;

    if ((s_fit_trend_acc_c[src_idx] >= 4) || (s_fit_trend_acc_c[src_idx] <= -4)) {
        trend_single = 1u;
    }

    if ((0u) && (trend_single != 0u) && (src_step != 0)) {
        if (src_idx <= 2u) {
            uint8_t i;
            for (i = 0u; i < 3u; i++) {
                int16_t peer;
                int16_t peer_step;
                int16_t dpeer;
                if ((i == src_idx) || (IsMainTempInvalid(system_data.pt1000_temp[i]) != 0u) || (s_fit_prev_valid[i] == 0u)) {
                    continue;
                }
                peer = TempToC(system_data.pt1000_temp[i]);
                peer_step = (int16_t)(peer - s_fit_prev_c[i]);
                dpeer = (src_c >= peer) ? (src_c - peer) : (int16_t)(peer - src_c);
                if (((src_step > 0) && (peer_step > 0)) || ((src_step < 0) && (peer_step < 0))) {
                    if (dpeer <= FIT_MAIN_OUTLIER_DIFF_C) {
                        trend_peer_ok = 1u;
                        break;
                    }
                }
            }
        } else {
            int16_t t[3];
            uint8_t vcnt = 0u;
            if (IsMainTempInvalid(system_data.pt1000_temp[0]) == 0u) { t[vcnt++] = TempToC(system_data.pt1000_temp[0]); }
            if (IsMainTempInvalid(system_data.pt1000_temp[1]) == 0u) { t[vcnt++] = TempToC(system_data.pt1000_temp[1]); }
            if (IsMainTempInvalid(system_data.pt1000_temp[2]) == 0u) { t[vcnt++] = TempToC(system_data.pt1000_temp[2]); }
            if (vcnt >= 2u) {
                int16_t ref;
                int16_t ref_prev;
                int16_t ref_step;
                int16_t dref;
                if (vcnt == 2u) {
                    ref = (int16_t)(((int32_t)t[0] + (int32_t)t[1]) / 2);
                } else {
                    if (t[0] > t[1]) { int16_t x = t[0]; t[0] = t[1]; t[1] = x; }
                    if (t[1] > t[2]) { int16_t x = t[1]; t[1] = t[2]; t[2] = x; }
                    if (t[0] > t[1]) { int16_t x = t[0]; t[0] = t[1]; t[1] = x; }
                    ref = t[1];
                }
                ref_prev = ref;
                if ((s_fit_prev_valid[0] != 0u) && (IsMainTempInvalid(system_data.pt1000_temp[0]) == 0u) &&
                    (s_fit_prev_valid[1] != 0u) && (IsMainTempInvalid(system_data.pt1000_temp[1]) == 0u)) {
                    if (vcnt == 2u) {
                        ref_prev = (int16_t)(((int32_t)s_fit_prev_c[0] + (int32_t)s_fit_prev_c[1]) / 2);
                    } else {
                        int16_t p[3];
                        p[0] = s_fit_prev_c[0];
                        p[1] = s_fit_prev_c[1];
                        p[2] = s_fit_prev_c[2];
                        if (p[0] > p[1]) { int16_t x = p[0]; p[0] = p[1]; p[1] = x; }
                        if (p[1] > p[2]) { int16_t x = p[1]; p[1] = p[2]; p[2] = x; }
                        if (p[0] > p[1]) { int16_t x = p[0]; p[0] = p[1]; p[1] = x; }
                        ref_prev = p[1];
                    }
                }
                ref_step = (int16_t)(ref - ref_prev);
                dref = (src_c >= ref) ? (src_c - ref) : (int16_t)(ref - src_c);
                if ((((src_step > 0) && (ref_step > 0)) || ((src_step < 0) && (ref_step < 0))) &&
                    (dref <= FIT_OUTLIER_DIFF_C)) {
                    trend_peer_ok = 1u;
                }
            }
        }

        if (trend_peer_ok == 0u) {
            *out_display_temp = raw_temp;
            return 1u;
        }
    }

    // First apply main-channel consensus correction; if applied, finish immediately.
    if (CalcMainConsensusTemp(src_idx, raw_temp, out_display_temp) != 0u) {
        return 1u;
    }

    // Fusion requires valid output pointer, connected probe, and no probe over-range alarms.
    if ((g_probe_connected == 0u) || (g_probe_over_hi != 0u) || (g_probe_over_lo != 0u)) {
        return 0u;
    }
    // Probe channel must contain a valid temperature sample.
    if (IsMainTempInvalid(system_data.pt1000_temp[3]) != 0u) {
        return 0u;
    }

    // Convert probe to Celsius so fusion thresholds are unit-independent.
    probe = TempToC(system_data.pt1000_temp[3]);
    // Disable fusion at high temperature region to avoid cross-channel bias under large gradients.
    if (probe >= FIT_TEMP_C_THRESHOLD) {
        return 0u;
    }

    // Convert all main channels to Celsius for outlier gating against probe.
    t0 = TempToC(system_data.pt1000_temp[0]);
    t1 = TempToC(system_data.pt1000_temp[1]);
    t2 = TempToC(system_data.pt1000_temp[2]);

    // Start with probe as strongest contributor.
    wsum = (int32_t)probe * 3; /* probe weight highest */
    w = 3;

    // Include main channel 0 only when close enough to probe; otherwise keep source standalone if it is channel 0.
    if (IsMainTempInvalid(system_data.pt1000_temp[0]) == 0u) {
        d = (t0 >= probe) ? (t0 - probe) : (int16_t)(probe - t0);
        if (d <= FIT_OUTLIER_DIFF_C) { wsum += t0; w++; }
        else if (src_idx == 0u) { *out_display_temp = raw_temp; return 1u; }
    }
    // Include main channel 1 only when close enough to probe; otherwise keep source standalone if it is channel 1.
    if (IsMainTempInvalid(system_data.pt1000_temp[1]) == 0u) {
        d = (t1 >= probe) ? (t1 - probe) : (int16_t)(probe - t1);
        if (d <= FIT_OUTLIER_DIFF_C) { wsum += t1; w++; }
        else if (src_idx == 1u) { *out_display_temp = raw_temp; return 1u; }
    }
    // Include main channel 2 only when close enough to probe; otherwise keep source standalone if it is channel 2.
    if (IsMainTempInvalid(system_data.pt1000_temp[2]) == 0u) {
        d = (t2 >= probe) ? (t2 - probe) : (int16_t)(probe - t2);
        if (d <= FIT_OUTLIER_DIFF_C) { wsum += t2; w++; }
        else if (src_idx == 2u) { *out_display_temp = raw_temp; return 1u; }
    }

    // If no main channel joined, return raw source as standalone output.
    if (w <= 3) {
        *out_display_temp = raw_temp; /* all main channels outlier -> standalone */
        return 1u;
    }

    // Output weighted average converted back to current display unit.
    *out_display_temp = TempFromC((int16_t)(wsum / w));
    return 1u;
}

/**
  * @function SysDataInit()
  * -------------
  * @brief    Initialize runtime system state after power-on.
  * @param    None
  * @note     None
  */
void SysDataInit(void)
{
    uint8_t i;

    Power_On;
    BT_Off;
    s_bt_hw_on = 0u;
    s_bt_state = BT_OFF;
    s_bt_icon_on = 0u;
    s_bt_pairing_req = 0u;
    s_bt_poweroff_req = 0u;
    s_bt_connected_input = 0u;
    work_process = start_up;
    system_data.units = unitC;
    for (i = 0u; i < 9u; i++) {
        system_data.pt1000_temp[i] = HaveTempErr;
    }
    DisplayMode = DISPLAY_MODE_D_SURFACE;
    delay_ms(500);
}

/**
  * @function UI_NotifyLocalInteraction()
  * ---------------------------
  * @brief    Notify UI logic of local user interaction.
  * @param    None
  * @note     None
  */
void UI_NotifyLocalInteraction(void)
{
    s_idle_ticks_100ms = 0u;
}

/**
  * @function UI_CycleDisplayMode()
  * ---------------------
  * @brief    Cycle through available display modes.
  * @param    None
  * @note     None
  */
void UI_CycleDisplayMode(void)
{
    DisplayMode_t next = DisplayMode;
    uint8_t no_main_temp;

    no_main_temp = (IsMainChannelDisconnected(system_data.pt1000_temp[0]) != 0u) &&
                   (IsMainChannelDisconnected(system_data.pt1000_temp[1]) != 0u) &&
                   (IsMainChannelDisconnected(system_data.pt1000_temp[2]) != 0u);
    if (no_main_temp != 0u) {
        return;
    }

    if (DisplayMode == DISPLAY_MODE_O_SURFACE) {
        next = DISPLAY_MODE_D_SURFACE;
    } else if (DisplayMode == DISPLAY_MODE_D_SURFACE) {
        next = DISPLAY_MODE_CAVITY;
    } else if (DisplayMode == DISPLAY_MODE_CAVITY) {
        next = g_probe_connected ? DISPLAY_MODE_PROBE : DISPLAY_MODE_O_SURFACE;
    } else {
        next = DISPLAY_MODE_O_SURFACE;
    }

    DisplayMode = next;
}

/**
  * @function UI_RequestBluetoothPairing()
  * ----------------------------
  * @brief    Request Bluetooth pairing mode.
  * @param    None
  * @note     None
  */
void UI_RequestBluetoothPairing(void)
{
    s_bt_pairing_req = 1u;
    BluetoothHwPowerOn();
}

/**
  * @function UI_RequestBluetoothPowerOff()
  * -----------------------------
  * @brief    Request Bluetooth power-off.
  * @param    None
  * @note     None
  */
void UI_RequestBluetoothPowerOff(void)
{
    s_bt_poweroff_req = 1u;
    BluetoothHwPowerOff();
}

/**
  * @function UI_SetBluetoothConnectionState()
  * --------------------------------
  * @brief    Update Bluetooth link state input.
  * @param    connected - input parameter
  * @note     None
  */
void UI_SetBluetoothConnectionState(uint8_t connected)
{
    if (connected != 0u) {
        s_bt_connected_input = 1u;
    } else {
        s_bt_connected_input = 0u;
    }
}

/**
  * @function UI_GetDisplayTemp()
  * -------------------
  * @brief    Get current cached display temperature.
  * @param    None
  * @note     None
  */
int16_t UI_GetDisplayTemp(void)
{
    return s_display_temp;
}

/**
  * @function UI_GetDisplaySpecial()
  * ----------------------
  * @brief    Get current special display status.
  * @param    None
  * @note     None
  */
uint8_t UI_GetDisplaySpecial(void)
{
    return s_display_special;
}

/**
  * @function UI_GetBluetoothIconState()
  * --------------------------
  * @brief    Get current Bluetooth icon visibility state.
  * @param    None
  * @note     None
  */
uint8_t UI_GetBluetoothIconState(void)
{
    return s_bt_icon_on;
}

/**
  * @function UI_IsProbeConnected()
  * ---------------------
  * @brief    Check whether probe is currently considered connected.
  * @param    None
  * @note     None
  */
uint8_t UI_IsProbeConnected(void)
{
    return g_probe_connected;
}

/**
  * @function LedTask()
  * ------------
  * @brief    Execute periodic LED task.
  * @param    None
  * @note     None
  */
static void LedTask(void)
{
    run_led_toggle;
}

/**
  * @function BluetoothHwPowerOn()
  * --------------------
  * @brief    Power on Bluetooth hardware block.
  * @param    None
  * @note     None
  */
static void BluetoothHwPowerOn(void)
{
    if (s_bt_hw_on == 0u) {
        BT_On;
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
        BT_Reset;
        s_bt_hw_on = 1u;
    }
}

/**
  * @function BluetoothHwPowerOff()
  * ---------------------
  * @brief    Power off Bluetooth hardware block.
  * @param    None
  * @note     None
  */
static void BluetoothHwPowerOff(void)
{
    if (s_bt_hw_on != 0u) {
        BT_Off;
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
        s_bt_hw_on = 0u;
    }
}

/**
  * @function BluetoothTask_100ms()
  * ---------------------
  * @brief    Run 100 ms Bluetooth state-machine tick.
  * @param    None
  * @note     None
  */
static void BluetoothTask_100ms(void)
{
    if (s_bt_poweroff_req != 0u) {
        s_bt_state = BT_OFF;
        s_bt_icon_on = 0u;
        s_bt_blink_ticks_100ms = 0u;
        s_bt_pairing_ticks_100ms = 0u;
        s_bt_poweroff_req = 0u;
        s_bt_pairing_req = 0u;
        s_bt_connected_input = 0u;
    }

    if (s_bt_pairing_req != 0u) {
        s_bt_state = BT_PAIRING;
        s_bt_pairing_ticks_100ms = 0u;
        s_bt_blink_ticks_100ms = 0u;
        s_bt_icon_on = 1u;
        s_bt_pairing_req = 0u;
        BluetoothHwPowerOn();
    }

    if ((s_bt_state == BT_PAIRING) || (s_bt_state == BT_RECONNECTING)) {
        BluetoothHwPowerOn();
        if (s_bt_connected_input != 0u) {
            s_bt_state = BT_CONNECTED;
            s_bt_icon_on = 1u;
            s_bt_blink_ticks_100ms = 0u;
        }
    } else if (s_bt_state == BT_CONNECTED) {
        if (s_bt_connected_input == 0u) {
            s_bt_state = BT_RECONNECTING;
            s_bt_blink_ticks_100ms = 0u;
        }
    } else {
        BluetoothHwPowerOff();
    }

    if ((s_bt_state == BT_PAIRING) || (s_bt_state == BT_RECONNECTING)) {
        s_bt_blink_ticks_100ms++;
        if (s_bt_blink_ticks_100ms >= 5u) {
            s_bt_blink_ticks_100ms = 0u;
            s_bt_icon_on = (uint8_t)!s_bt_icon_on;
        }
    } else if (s_bt_state == BT_CONNECTED) {
        s_bt_icon_on = 1u;
    } else {
        s_bt_icon_on = 0u;
    }

    if (s_bt_state == BT_PAIRING) {
        s_bt_pairing_ticks_100ms++;
        if (s_bt_pairing_ticks_100ms >= 1200u) {
            s_bt_state = BT_OFF;
            s_bt_icon_on = 0u;
            s_bt_connected_input = 0u;
        }
    }
}

/**
  * @function UpdateDisplayCache()
  * --------------------
  * @brief    Refresh display cache from latest temperature states.
  * @param    None
  * @note     None
  */
static void UpdateDisplayCache(void)
{
    uint8_t no_main_temp;
    int16_t ch_temp;
    int16_t fitted_temp;

    s_display_special = 0u;
    no_main_temp = (IsMainChannelDisconnected(system_data.pt1000_temp[0]) != 0u) &&
                   (IsMainChannelDisconnected(system_data.pt1000_temp[1]) != 0u) &&
                   (IsMainChannelDisconnected(system_data.pt1000_temp[2]) != 0u);

    if ((no_main_temp != 0u) && (g_probe_connected != 0u)) {
        DisplayMode = DISPLAY_MODE_PROBE;
    }

    if ((no_main_temp != 0u) && !((DisplayMode == DISPLAY_MODE_PROBE) && (g_probe_connected != 0u))) {
        s_display_special = 3u;
        s_display_temp = 0;
        DisplayMode = DISPLAY_MODE_D_SURFACE;
        return;
    }

    switch (DisplayMode)
    {
        case DISPLAY_MODE_O_SURFACE:
            ch_temp = system_data.pt1000_temp[1];
            if (ApplyTempSpecial(ch_temp) == 0u) {
                if (CalcFittedTemp(1u, ch_temp, &fitted_temp) != 0u) { s_display_temp = fitted_temp; }
                else { s_display_temp = ch_temp; }
						
            }
            break;

        case DISPLAY_MODE_D_SURFACE:
            ch_temp = system_data.pt1000_temp[0];
            if (ApplyTempSpecial(ch_temp) == 0u) {
                if (CalcFittedTemp(0u, ch_temp, &fitted_temp) != 0u) { s_display_temp = fitted_temp; }
                else { s_display_temp = ch_temp; }
						
            }
            break;

        case DISPLAY_MODE_CAVITY:
            ch_temp = system_data.pt1000_temp[2];
            if (ApplyTempSpecial(ch_temp) == 0u) {
                if (CalcFittedTemp(2u, ch_temp, &fitted_temp) != 0u) { s_display_temp = fitted_temp; }
                else { s_display_temp = ch_temp; }
		
            }
            break;

        case DISPLAY_MODE_PROBE:
            if (g_probe_connected == 0u) {
                s_display_special = 3u;
                s_display_temp = 0;
            } else if (g_probe_over_hi != 0u) {
                s_display_special = 1u;
                s_display_temp = 0;
            } else if (g_probe_over_lo != 0u) {
                s_display_special = 2u;
                s_display_temp = 0;
            } else {
                ch_temp = (int16_t)system_data.pt1000_temp[3];
                if (CalcFittedTemp(3u, ch_temp, &fitted_temp) != 0u) { s_display_temp = fitted_temp; }
                else { s_display_temp = ch_temp; }
							 
            }
            break;

        default:
            s_display_temp = (int16_t)system_data.pt1000_temp[0];
            break;
    }
}

/**
  * @function MainControl()
  * -------------
  * @brief    Execute main control loop task.
  * @param    None
  * @note     None
  */
void MainControl(void)
{
    uint8_t probe_now;
    uint8_t probe_was_absent_long;

    LedTask();
	
    TempGetTask();
	
    probe_now = g_probe_connected;

    KeyRespose(Key0_short_press, Key1_short_press, Key0_long_press, Key1_long_press, Key0_very_long_press, Key1_very_long_press);

    probe_was_absent_long = (s_probe_absent_ticks_100ms >= PROBE_REINSERT_MIN_OFF_TICKS_100MS) ? 1u : 0u;

    if ((work_process != idle) && (work_process != shutdown)) {
        if ((probe_now != 0u) && (s_probe_connected_last == 0u)) {
            if (probe_was_absent_long != 0u) {
                DisplayMode = DISPLAY_MODE_PROBE;
                UI_NotifyLocalInteraction(); /* probe inserted counts as local interaction */
            }
        } else if ((probe_now == 0u) && (s_probe_connected_last != 0u) && (DisplayMode == DISPLAY_MODE_PROBE)) {
            DisplayMode = DISPLAY_MODE_D_SURFACE;
            UI_NotifyLocalInteraction(); /* probe removed counts as local interaction */
        }
    }

    if (probe_now == 0u) {
        if (s_probe_absent_ticks_100ms < 255u) {
            s_probe_absent_ticks_100ms++;
        }
    } else {
        s_probe_absent_ticks_100ms = 0u;
    }

    s_probe_connected_last = probe_now;

    if (work_process != s_last_work_process) {
        if (work_process == start_up) {
            if (probe_now != 0u) {
                DisplayMode = DISPLAY_MODE_PROBE;
            } else {
                DisplayMode = DISPLAY_MODE_D_SURFACE;
            }
            s_idle_ticks_100ms = 0u;
        }

    if (work_process == shutdown) {
            s_bt_state = BT_OFF;
            s_bt_icon_on = 0u;
            Power_Off;
        }

        s_last_work_process = work_process;
    }

    BluetoothTask_100ms();
    UpdateDisplayCache();

    if ((work_process != idle) && (work_process != shutdown)) {
        if (s_bt_state != BT_CONNECTED) {
            s_idle_ticks_100ms++;
            if (s_idle_ticks_100ms >= IDLE_AUTO_SHUTDOWN_TICKS_100MS) {
                work_process = shutdown;
            }
        }
    } else {
        s_idle_ticks_100ms = 0u;
    }
		
        /* Two-step power-off:
         * 1) enter shutdown immediately (screen off),
         * 2) wait KEY0 release, then cut PB3 power latch. */
    if (work_process == shutdown) {
        if (key0 != 0u) {
            Power_Off;
            work_process = idle;
        }
    }
}

















