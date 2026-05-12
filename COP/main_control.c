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

#define IDLE_AUTO_SHUTDOWN_TICKS_100MS  (3000u)
#define FIT_TEMP_C_THRESHOLD             (50)
#define FIT_OUTLIER_DIFF_C               (5)
#define FIT_MAIN_OUTLIER_DIFF_C          (3)
#define FIT_MAIN_OUTLIER_HOLD_COUNT      (2u)
#define FIT_MAIN_DYNAMIC_STEP_C          (1)
#define FIT_MAIN_DYNAMIC_CONFIRM_COUNT   (2u)
#define PROBE_REINSERT_MIN_OFF_TICKS_100MS (5u)

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

static void BluetoothHwPowerOn(void);
static void BluetoothHwPowerOff(void);

/**
 * @brief Check whether a main temperature sample is invalid.
 * @details Treats HaveTempErr sentinel as invalid and all other values as valid.
 * @param t Temperature sample in current display unit.
 * @return 1 if invalid, otherwise 0.
 */
static uint8_t IsMainTempInvalid(int16_t t)
{
    if (t == HaveTempErr) {
        return 1u;
    }
    return 0u;
}

/**
 * @brief Convert temperature from current UI unit to Celsius.
 * @details Passes through value in Celsius mode and converts from Fahrenheit otherwise.
 * @param t Temperature in current system unit.
 * @return Temperature converted to Celsius.
 */
static int16_t TempToC(int16_t t)
{
    if (system_data.units == unitC) {
        return t;
    }
    return (int16_t)((((int32_t)t - 32) * 5) / 9);
}

/**
 * @brief Convert temperature from Celsius to current UI unit.
 * @details Passes through value in Celsius mode and converts to Fahrenheit otherwise.
 * @param c Temperature in Celsius.
 * @return Temperature converted to current system unit.
 */
static int16_t TempFromC(int16_t c)
{
    if (system_data.units == unitC) {
        return c;
    }
    return (int16_t)(((int32_t)c * 9) / 5 + 32);
}


/**
 * @brief Apply main-channel consensus correction for outlier suppression.
 * @details Compares source channel against median/mean of valid main channels and optionally outputs corrected value.
 * @param src_idx Source channel index (0..2).
 * @param raw_temp Raw source temperature in current unit.
 * @param out_display_temp Output pointer for corrected display value.
 * @return 1 when correction is applied, otherwise 0.
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
        // Increase outlier hold count with saturation.
        if (s_main_outlier_hold_cnt[src_idx] < 255u) {
            s_main_outlier_hold_cnt[src_idx]++;
        }
        // Keep forcing consensus output until outlier persists long enough and dynamic evidence is sufficient.
        if ((s_main_outlier_hold_cnt[src_idx] <= FIT_MAIN_OUTLIER_HOLD_COUNT) ||
            (s_main_outlier_dynamic_cnt[src_idx] < FIT_MAIN_DYNAMIC_CONFIRM_COUNT)) {
            *out_display_temp = TempFromC(ref);
            return 1u;
        }
        // Outlier has enough persistence and dynamics, so allow raw source to pass through.
        return 0u;
    }

    // Source returned to consensus range, so clear outlier tracking state.
    s_main_outlier_hold_cnt[src_idx] = 0u;
    s_main_outlier_dynamic_cnt[src_idx] = 0u;
    return 0u;
}

/**
 * @brief Calculate fitted display temperature using probe and main channels.
 * @details Applies main consensus first, then performs weighted fusion with probe under valid and low-temperature conditions.
 * @param src_idx Source channel index.
 * @param raw_temp Raw source temperature in current unit.
 * @param out_display_temp Output pointer for fitted value.
 * @return 1 when fitted/corrected output is produced, otherwise 0.
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

    // First apply main-channel consensus correction; if applied, finish immediately.
    if (CalcMainConsensusTemp(src_idx, raw_temp, out_display_temp) != 0u) {
        return 1u;
    }

    // Fusion requires valid output pointer, connected probe, and no probe over-range alarms.
    if ((out_display_temp == 0) || (g_probe_connected == 0u) || (g_probe_over_hi != 0u) || (g_probe_over_lo != 0u)) {
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
 * @brief Initialize runtime system state after power-on.
 * @details Sets default work mode, unit, display mode, and Bluetooth-related state flags.
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
 * @brief Notify UI logic of local user interaction.
 * @details Resets idle timer used by auto-shutdown logic.
 */
void UI_NotifyLocalInteraction(void)
{
    s_idle_ticks_100ms = 0u;
}

/**
 * @brief Cycle through available display modes.
 * @details Skips probe mode when probe is absent and blocks mode switching if all main channels are invalid.
 */
void UI_CycleDisplayMode(void)
{
    DisplayMode_t next = DisplayMode;
    uint8_t no_main_temp;

    no_main_temp = (IsMainTempInvalid(system_data.pt1000_temp[0]) != 0u) &&
                   (IsMainTempInvalid(system_data.pt1000_temp[1]) != 0u) &&
                   (IsMainTempInvalid(system_data.pt1000_temp[2]) != 0u);
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
 * @brief Request Bluetooth pairing mode.
 * @details Sets pairing request flag and ensures Bluetooth hardware power is enabled.
 */
void UI_RequestBluetoothPairing(void)
{
    s_bt_pairing_req = 1u;
    BluetoothHwPowerOn();
}

/**
 * @brief Request Bluetooth power-off.
 * @details Sets power-off request flag and immediately powers hardware down path.
 */
void UI_RequestBluetoothPowerOff(void)
{
    s_bt_poweroff_req = 1u;
    BluetoothHwPowerOff();
}

/**
 * @brief Update Bluetooth link state input.
 * @details Stores external connection state for Bluetooth state machine processing.
 * @param connected Non-zero means connected.
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
 * @brief Get current cached display temperature.
 * @details Returns numeric temperature field used by UI rendering.
 * @return Cached display temperature.
 */
int16_t UI_GetDisplayTemp(void)
{
    return s_display_temp;
}

/**
 * @brief Get current special display status.
 * @details Returns special flag for -HI, -LO, or --- display conditions.
 * @return Special display code.
 */
uint8_t UI_GetDisplaySpecial(void)
{
    return s_display_special;
}

/**
 * @brief Get current Bluetooth icon visibility state.
 * @details Used by UI to render steady/blinking/off Bluetooth indicator.
 * @return Icon on/off state.
 */
uint8_t UI_GetBluetoothIconState(void)
{
    return s_bt_icon_on;
}

/**
 * @brief Check whether probe is currently considered connected.
 * @details Returns debounced probe connection state.
 * @return 1 if connected, otherwise 0.
 */
uint8_t UI_IsProbeConnected(void)
{
    return g_probe_connected;
}

/**
 * @brief Execute periodic LED task.
 * @details Toggles run indicator LED.
 */
static void LedTask(void)
{
    run_led_toggle;
}

/**
 * @brief Power on Bluetooth hardware block.
 * @details Enables Bluetooth power pin and applies reset sequence once.
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
 * @brief Power off Bluetooth hardware block.
 * @details Disables Bluetooth power and clears hardware-on tracking flag.
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
 * @brief Run 100 ms Bluetooth state-machine tick.
 * @details Handles pairing, reconnecting, link transitions, icon blinking, and pairing timeout.
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
 * @brief Refresh display cache from latest temperature states.
 * @details Selects channel by mode, applies fitted temperature logic, and updates special display flags.
 */
static void UpdateDisplayCache(void)
{
    uint8_t no_main_temp;
    int16_t ch_temp;
    int16_t fitted_temp;

    s_display_special = 0u;
    no_main_temp = (IsMainTempInvalid(system_data.pt1000_temp[0]) != 0u) &&
                   (IsMainTempInvalid(system_data.pt1000_temp[1]) != 0u) &&
                   (IsMainTempInvalid(system_data.pt1000_temp[2]) != 0u);

    if (no_main_temp != 0u) {
        s_display_special = 3u;
        s_display_temp = 0;
        return;
    }

    switch (DisplayMode)
    {
        case DISPLAY_MODE_O_SURFACE:
            ch_temp = system_data.pt1000_temp[1];
            if (IsMainTempInvalid(ch_temp) != 0u) {
                s_display_special = 3u;
                s_display_temp = 0;
            } else {
                if (CalcFittedTemp(1u, ch_temp, &fitted_temp) != 0u) { s_display_temp = fitted_temp; }
                else { s_display_temp = ch_temp; }
						
            }
            break;

        case DISPLAY_MODE_D_SURFACE:
            ch_temp = system_data.pt1000_temp[0];
            if (IsMainTempInvalid(ch_temp) != 0u) {
                s_display_special = 3u;
                s_display_temp = 0;
            } else {
                if (CalcFittedTemp(0u, ch_temp, &fitted_temp) != 0u) { s_display_temp = fitted_temp; }
                else { s_display_temp = ch_temp; }
						
            }
            break;

        case DISPLAY_MODE_CAVITY:
            ch_temp = system_data.pt1000_temp[2];
            if (IsMainTempInvalid(ch_temp) != 0u) {
                s_display_special = 3u;
                s_display_temp = 0;
            } else {
                if (CalcFittedTemp(2u, ch_temp, &fitted_temp) != 0u) { s_display_temp = fitted_temp; }
                else { s_display_temp = ch_temp; }
		
            }
            break;

        case DISPLAY_MODE_PROBE:
            if (g_probe_connected == 0u) {
                s_display_temp = (int16_t)system_data.pt1000_temp[0];
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
 * @brief Main control loop task.
 * @details Runs sensor acquisition, key handling, mode transitions, UI cache updates, and process state actions.
 */
void MainControl(void)
{
    uint8_t probe_now;
    uint8_t probe_was_absent_long;

    LedTask();
	
    TempGetTask();
	
    probe_now = g_probe_connected;

    if (probe_now == 0u) {
        DisplayMode = DISPLAY_MODE_D_SURFACE;
    }

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

















