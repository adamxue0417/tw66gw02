/**
  ******************************************************************************
  * @file    main_control.c
  * @author 
  * @version V1.0
  * @date
  * @brief   Main control state machine and UI runtime cache implementation.
  ******************************************************************************
  */
#include "main_control.h"
#include "config.h"
#include "ota_layout.h"
#include "mathis_util.h"
#include <stddef.h>

_work_process work_process,work_process_backups;
_system_data  system_data;

volatile uint8_t g_probe_connected = 0u;
volatile uint8_t g_probe_over_hi = 0u;
volatile uint8_t g_probe_over_lo = 0u;





static uint8_t s_bt_state = BT_OFF;
static uint8_t s_bt_icon_on = 0u;
static uint8_t s_bt_pairing_req = 0u;
static uint8_t s_bt_poweroff_req = 0u;
static uint8_t s_bt_connected_input = 0u;
static uint8_t s_bt_hw_on = 0u;
static uint16_t s_bt_blink_ticks_100ms = 0u;
static uint16_t s_bt_pairing_ticks_100ms = 0u;
static uint32_t s_idle_ticks_100ms = 0u;

#define IDLE_AUTO_SHUTDOWN_TICKS_100MS (36000u)
#define CONFIG_PAGE_A_ADDR              (OTA_CONFIG_A_BASE)
#define CONFIG_PAGE_B_ADDR              (OTA_CONFIG_B_BASE)
#define CONFIG_MAGIC                    (0x4D415448u)
#define CONFIG_VERSION                  (1u)
#define CONFIG_COMMIT                   (0xA55Au)

static int16_t s_display_temp = 0;
/* 0:none, 1:-HI, 2:-LO, 3:--- */
static uint8_t s_display_special = 3u;

static _work_process s_last_work_process = idle;
static uint8_t s_probe_connected_last = 0u;
static uint8_t s_probe_absent_ticks_100ms = 0u;
static float s_temp_coeff[TEMP_COEFF_CHANNEL_COUNT][TEMP_COEFF_TERM_COUNT] =
{
    {0.0f, 0.0f, 0.0f, 1.0f, 0.0f}, /* D surface: y = x by default */
    {0.0f, 0.0f, 0.0f, 1.0f, 0.0f}, /* O surface: y = x by default */
    {0.0f, 0.0f, 0.0f, 1.0f, 0.0f}  /* Cavity: y = x by default */
};

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t length;
    uint32_t sequence;
    uint8_t units;
    uint8_t reserved[3];
    float coeff[TEMP_COEFF_CHANNEL_COUNT][TEMP_COEFF_TERM_COUNT];
    uint16_t crc;
    uint16_t commit;
} PersistedConfig;

static int16_t s_adjusted_raw[TEMP_COEFF_CHANNEL_COUNT];
static int16_t s_adjusted_value[TEMP_COEFF_CHANNEL_COUNT];
static uint8_t s_adjusted_valid[TEMP_COEFF_CHANNEL_COUNT];
static uint32_t s_config_sequence = 0u;
static uint32_t s_config_active_page = 0u;

static void BluetoothHwPowerOn(void);
static void BluetoothHwPowerOff(void);

static uint16_t ConfigCrc16(const uint8_t *data, uint16_t length)
{
    return Mathis_Crc16Bytes(data, length);
}

static uint8_t ConfigRecordValid(const PersistedConfig *cfg)
{
    uint8_t channel, term;
    for (channel = 0u; channel < TEMP_COEFF_CHANNEL_COUNT; channel++) {
        for (term = 0u; term < TEMP_COEFF_TERM_COUNT; term++) {
            if (Mathis_FloatFinite(cfg->coeff[channel][term]) == 0u) { return 0u; }
        }
    }
    if ((cfg->magic != CONFIG_MAGIC) || (cfg->version != CONFIG_VERSION) ||
        (cfg->length != sizeof(PersistedConfig)) || (cfg->commit != CONFIG_COMMIT) ||
        ((cfg->units != unitC) && (cfg->units != unitF))) { return 0u; }
    return (ConfigCrc16((const uint8_t *)cfg, (uint16_t)offsetof(PersistedConfig, crc)) == cfg->crc) ? 1u : 0u;
}

static void ConfigLoad(void)
{
    const PersistedConfig *a = (const PersistedConfig *)CONFIG_PAGE_A_ADDR;
    const PersistedConfig *b = (const PersistedConfig *)CONFIG_PAGE_B_ADDR;
    const PersistedConfig *selected = 0;
    uint8_t va = ConfigRecordValid(a);
    uint8_t vb = ConfigRecordValid(b);
    if ((va != 0u) && (vb != 0u)) { selected = ((int32_t)(b->sequence - a->sequence) > 0) ? b : a; }
    else if (va != 0u) { selected = a; }
    else if (vb != 0u) { selected = b; }
    if (selected != 0) {
        system_data.units = selected->units;
        memcpy(s_temp_coeff, selected->coeff, sizeof(s_temp_coeff));
        memset(s_adjusted_valid, 0, sizeof(s_adjusted_valid));
        s_config_sequence = selected->sequence;
        s_config_active_page = (selected == a) ? CONFIG_PAGE_A_ADDR : CONFIG_PAGE_B_ADDR;
    }
}

static uint8_t ConfigSave(void)
{
    PersistedConfig cfg;
    FLASH_EraseInitTypeDef erase;
    uint32_t page_error = 0u;
    uint32_t target;
    uint16_t offset;
    const uint16_t *src = (const uint16_t *)&cfg;
    if (s_config_active_page != 0u) {
        const PersistedConfig *active = (const PersistedConfig *)s_config_active_page;
        if ((ConfigRecordValid(active) != 0u) && (active->units == system_data.units) &&
            (memcmp(active->coeff, s_temp_coeff, sizeof(s_temp_coeff)) == 0)) { return 1u; }
    }
    memset(&cfg, 0xFF, sizeof(cfg));
    cfg.magic = CONFIG_MAGIC; cfg.version = CONFIG_VERSION; cfg.length = sizeof(PersistedConfig);
    cfg.sequence = s_config_sequence + 1u; cfg.units = system_data.units;
    memcpy(cfg.coeff, s_temp_coeff, sizeof(s_temp_coeff));
    cfg.crc = ConfigCrc16((const uint8_t *)&cfg, (uint16_t)offsetof(PersistedConfig, crc));
    cfg.commit = CONFIG_COMMIT;
    target = (s_config_active_page == CONFIG_PAGE_A_ADDR) ? CONFIG_PAGE_B_ADDR : CONFIG_PAGE_A_ADDR;
    erase.TypeErase = FLASH_TYPEERASE_PAGES; erase.PageAddress = target; erase.NbPages = 1u;
    if (HAL_FLASH_Unlock() != HAL_OK) { return 0u; }
    if (HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK) { HAL_FLASH_Lock(); return 0u; }
    for (offset = 0u; offset < (uint16_t)offsetof(PersistedConfig, commit); offset += 2u) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, target + offset, src[offset / 2u]) != HAL_OK) {
            HAL_FLASH_Lock(); return 0u;
        }
    }
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, target + (uint32_t)offsetof(PersistedConfig, commit), cfg.commit) != HAL_OK) {
        HAL_FLASH_Lock(); return 0u;
    }
    HAL_FLASH_Lock();
    if ((ConfigRecordValid((const PersistedConfig *)target) == 0u) ||
        (memcmp((const void *)target, &cfg, sizeof(cfg)) != 0)) { return 0u; }
    s_config_sequence = cfg.sequence; s_config_active_page = target;
    return 1u;
}

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
  * @brief    Return the cached Celsius temperature.
  * @param    t - input parameter
  * @note     None
  */
static int16_t TempToC(int16_t t)
{
    return t;
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
    int32_t scaled;

    if (system_data.units == unitC) {
        return c;
    }

    /* Round C * 9 / 5 to nearest, with half values away from zero. */
    scaled = (int32_t)c * 9;
    scaled = (scaled >= 0) ? ((scaled + 2) / 5) : ((scaled - 2) / 5);
    return (int16_t)(scaled + 32);
}

static int16_t RoundFloatToInt16(float v)
{
    if (Mathis_FloatFinite(v) == 0u) { return HaveTempErr; }
    if (v > 32767.0f) { return 32767; }
    if (v < -32768.0f) { return -32768; }
    if (v >= 0.0f) { return (int16_t)(v + 0.5f); }
    return (int16_t)(v - 0.5f);
}

static int16_t ApplyMainTempCoefficient(uint8_t src_idx, int16_t raw_temp)
{
    float x;
    float y;

    if ((src_idx >= TEMP_COEFF_CHANNEL_COUNT) || (IsMainTempInvalid(raw_temp) != 0u)) {
        return raw_temp;
    }

    x = (float)TempToC(raw_temp);
    y = (((s_temp_coeff[src_idx][0] * x + s_temp_coeff[src_idx][1]) * x +
          s_temp_coeff[src_idx][2]) * x + s_temp_coeff[src_idx][3]) * x +
          s_temp_coeff[src_idx][4];

    if (Mathis_FloatFinite(y) == 0u) { return HaveTempErr; }
    if (y > 500.0f) { return TempHigh; }
    if (y < 0.0f) { return TempLow; }

    return RoundFloatToInt16(y);
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
    ConfigLoad();
    s_display_temp = 0;
    s_display_special = 3u;
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

    if (DisplayMode == DISPLAY_MODE_PROBE) {
        next = DISPLAY_MODE_O_SURFACE;
    } else if (DisplayMode == DISPLAY_MODE_O_SURFACE) {
        next = DISPLAY_MODE_D_SURFACE;
    } else if (DisplayMode == DISPLAY_MODE_D_SURFACE) {
        next = DISPLAY_MODE_CAVITY;
    } else if (DisplayMode == DISPLAY_MODE_CAVITY) {
        next = g_probe_connected ? DISPLAY_MODE_PROBE : DISPLAY_MODE_O_SURFACE;
    } else {
        next = g_probe_connected ? DISPLAY_MODE_PROBE : DISPLAY_MODE_O_SURFACE;
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
  * @function UI_RequestBluetoothToggle()
  * -----------------------------
  * @brief    Toggle Bluetooth pairing/power-off from one long-press action.
  * @param    None
  * @note     None
  */
void UI_RequestBluetoothToggle(void)
{
    if ((s_bt_state == BT_OFF) && (s_bt_hw_on == 0u)) {
        UI_RequestBluetoothPairing();
    } else if (s_bt_state != BT_CONNECTED) {
        UI_RequestBluetoothPowerOff();
    }
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
        if (s_bt_connected_input == 0u) { UI_NotifyLocalInteraction(); }
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

int16_t UI_GetAdjustedMainTemp(uint8_t src_idx)
{
    int16_t raw;
    if (src_idx >= TEMP_COEFF_CHANNEL_COUNT) { return HaveTempErr; }
    raw = system_data.pt1000_temp[src_idx];
    if ((s_adjusted_valid[src_idx] == 0u) || (s_adjusted_raw[src_idx] != raw)) {
        s_adjusted_raw[src_idx] = raw;
        s_adjusted_value[src_idx] = ApplyMainTempCoefficient(src_idx, raw);
        s_adjusted_valid[src_idx] = 1u;
    }
    return s_adjusted_value[src_idx];
}

uint8_t UI_SetTempCoeff(uint8_t src_idx, const float coeff[TEMP_COEFF_TERM_COUNT])
{
    uint8_t i;
    float previous[TEMP_COEFF_TERM_COUNT];

    if ((src_idx >= TEMP_COEFF_CHANNEL_COUNT) || (coeff == 0)) {
        return 0u;
    }

    for (i = 0u; i < TEMP_COEFF_TERM_COUNT; i++) {
        if (Mathis_FloatFinite(coeff[i]) == 0u) { return 0u; }
    }
    for (i = 0u; i < TEMP_COEFF_TERM_COUNT; i++) {
        previous[i] = s_temp_coeff[src_idx][i];
        s_temp_coeff[src_idx][i] = coeff[i];
    }
    s_adjusted_valid[src_idx] = 0u;
    if (ConfigSave() == 0u) {
        for (i = 0u; i < TEMP_COEFF_TERM_COUNT; i++) { s_temp_coeff[src_idx][i] = previous[i]; }
        return 0u;
    }
    return 1u;
}

uint8_t UI_GetTempCoeff(uint8_t src_idx, float coeff[TEMP_COEFF_TERM_COUNT])
{
    uint8_t i;

    if ((src_idx >= TEMP_COEFF_CHANNEL_COUNT) || (coeff == 0)) {
        return 0u;
    }

    for (i = 0u; i < TEMP_COEFF_TERM_COUNT; i++) {
        coeff[i] = s_temp_coeff[src_idx][i];
    }
    return 1u;
}

uint8_t UI_SetUnits(uint8_t units, uint8_t persist)
{
    uint8_t previous;
    if ((units != unitC) && (units != unitF)) { return 0u; }
    if (system_data.units == units) { return 1u; }
    previous = system_data.units;
    system_data.units = units;
    if ((persist != 0u) && (ConfigSave() == 0u)) { system_data.units = previous; return 0u; }
    return 1u;
}

void UI_FactoryReset(void)
{
    uint8_t previous = system_data.units;
    system_data.units = unitC;
    if (ConfigSave() == 0u) { system_data.units = previous; }
    UI_NotifyLocalInteraction();
}

void UI_GetTelemetrySnapshot(MathisTelemetrySnapshot *snapshot)
{
    uint8_t i;
    static const uint8_t internal_index[3] = {2u, 1u, 0u};
    if (snapshot == 0) { return; }
    memset(snapshot, 0, sizeof(*snapshot));
    snapshot->units = system_data.units;
    for (i = 0u; i < 3u; i++) {
        int16_t raw = system_data.pt1000_temp[internal_index[i]];
        int16_t adjusted;
        if ((raw == HaveTempErr) || (raw == TempDisconnected)) { snapshot->error_code = 0x02u; continue; }
        adjusted = UI_GetAdjustedMainTemp(internal_index[i]);
        if (adjusted == HaveTempErr) { snapshot->error_code = 0x02u; continue; }
        if (adjusted == TempHigh) { snapshot->high_mask |= (uint8_t)(1u << i); continue; }
        if (adjusted == TempLow) { snapshot->low_mask |= (uint8_t)(1u << i); continue; }
        snapshot->temp_c[i] = adjusted; snapshot->valid_mask |= (uint8_t)(1u << i);
    }
    if (g_probe_connected == 0u) { snapshot->error_code = 0x02u; }
    else if (g_probe_over_hi != 0u) { snapshot->high_mask |= 0x08u; }
    else if (g_probe_over_lo != 0u) { snapshot->low_mask |= 0x08u; }
    else { snapshot->temp_c[3] = system_data.pt1000_temp[3]; snapshot->valid_mask |= 0x08u; }
    if (g_battery_mv <= 1100u) { snapshot->battery_percent = 0u; }
    else if (g_battery_mv >= 1500u) { snapshot->battery_percent = 100u; }
    else { snapshot->battery_percent = (uint8_t)(((uint32_t)(g_battery_mv - 1100u) * 100u) / 400u); }
    if (g_battery_low != 0u) { snapshot->error_code = 0x01u; }
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
                fitted_temp = UI_GetAdjustedMainTemp(1u);
                if (ApplyTempSpecial(fitted_temp) == 0u) { s_display_temp = TempFromC(fitted_temp); }
						
            }
            break;

        case DISPLAY_MODE_D_SURFACE:
            ch_temp = system_data.pt1000_temp[0];
            if (ApplyTempSpecial(ch_temp) == 0u) {
                fitted_temp = UI_GetAdjustedMainTemp(0u);
                if (ApplyTempSpecial(fitted_temp) == 0u) { s_display_temp = TempFromC(fitted_temp); }
						
            }
            break;

        case DISPLAY_MODE_CAVITY:
            ch_temp = system_data.pt1000_temp[2];
            if (ApplyTempSpecial(ch_temp) == 0u) {
                fitted_temp = UI_GetAdjustedMainTemp(2u);
                if (ApplyTempSpecial(fitted_temp) == 0u) { s_display_temp = TempFromC(fitted_temp); }
		
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
                s_display_temp = TempFromC(ch_temp);
							 
            }
            break;

        default:
            s_display_temp = TempFromC((int16_t)system_data.pt1000_temp[0]);
            break;
    }
}

uint8_t UI_GetBluetoothState(void)
{
    return s_bt_state;
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
            Power_On;
            DisplayMode = DISPLAY_MODE_D_SURFACE;
            s_idle_ticks_100ms = 0u;
        }

        if (work_process == shutdown) {
            s_bt_state = BT_OFF;
            s_bt_icon_on = 0u;
            BluetoothHwPowerOff();
            Power_Off;
        }

        s_last_work_process = work_process;
    }

    BluetoothTask_100ms();
    UpdateDisplayCache();

    if ((work_process != idle) && (work_process != shutdown)) {
        if (s_idle_ticks_100ms < IDLE_AUTO_SHUTDOWN_TICKS_100MS) { s_idle_ticks_100ms++; }
        if (s_idle_ticks_100ms >= IDLE_AUTO_SHUTDOWN_TICKS_100MS) { work_process = shutdown; }
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
















