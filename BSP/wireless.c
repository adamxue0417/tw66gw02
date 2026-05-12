#include "config.h"

/*
 * Wireless protocol compatible with TW15A01 AppTask style.
 * Transport: USART1 + COM1 DMA RX
 * Frame: [0]=0xFE, [1]=CMD, [...payload...], [last]=0xFF
 */

#define WL_HEAD                     0xFEu
#define WL_TAIL                     0xFFu
#define WL_TX_MAX                   64u

#define WL_CMD_POWER                0x01u
#define WL_CMD_TEMP_UP              0x03u
#define WL_CMD_TEMP_DOWN            0x04u
#define WL_CMD_SET_TEMP             0x05u
#define WL_CMD_JUMP_SET             0x06u
#define WL_CMD_QUERY_PROBE          0x07u
#define WL_CMD_UNIT                 0x09u
#define WL_CMD_QUERY_STATUS         0x0Bu
#define WL_CMD_QUERY_SET_ALL        0x0Du
#define WL_CMD_QUERY_ACT_ALL        0x0Eu
#define WL_CMD_BT_STATUS            0x24u
#define WL_CMD_FW_INFO              0x5Fu

#define WL_TEMP_MIN                 175u
#define WL_TEMP_MAX                 455u

static uint8_t s_tx[WL_TX_MAX];
static uint8_t s_bt_status = 0u;

static uint16_t wl_get_probe_temp(void)
{
    if (g_probe_connected == 0u) {
        return HaveTempErr;
    }
    return system_data.pt1000_temp[3];
}

static uint16_t wl_get_temp_by_channel(uint8_t ch)
{
    if (ch == 0x01u) { return system_data.pt1000_temp[3]; } /* probe1 in legacy protocol */
    if (ch == 0x02u) { return HaveTempErr; }                /* probe2 unsupported */
    return HaveTempErr;
}

static uint16_t wl_get_display_main_temp(void)
{
    if (DisplayMode == DISPLAY_MODE_O_SURFACE) { return system_data.pt1000_temp[1]; }
    if (DisplayMode == DISPLAY_MODE_D_SURFACE) { return system_data.pt1000_temp[0]; }
    if (DisplayMode == DISPLAY_MODE_CAVITY)    { return system_data.pt1000_temp[2]; }
    if (DisplayMode == DISPLAY_MODE_PROBE)     { return wl_get_probe_temp(); }
    return system_data.pt1000_temp[0];
}

static void wl_split3(uint16_t v, uint8_t *h, uint8_t *t, uint8_t *l)
{
    if (v > 999u) { v = 999u; }
    *h = (uint8_t)(v / 100u);
    *t = (uint8_t)((v / 10u) % 10u);
    *l = (uint8_t)(v % 10u);
}

static void wl_send(const uint8_t *buf, uint16_t len)
{
    if ((buf == NULL) || (len == 0u)) {
        return;
    }
    USART1_SendData((uint8_t *)buf, len);
}

static void wl_ack_cmd2(uint8_t cmd, uint8_t p2)
{
    s_tx[0] = WL_HEAD;
    s_tx[1] = cmd;
    s_tx[2] = p2;
    s_tx[3] = WL_TAIL;
    wl_send(s_tx, 4u);
}

static void wl_handle_power(const uint8_t *rx, uint16_t len)
{
    (void)len;
    if (rx[2] == 0x01u) {
        if (work_process == idle) { work_process = start_up; }
    } else if (rx[2] == 0x02u) {
        if (work_process != idle) { work_process = shutdown; }
    }
    UI_NotifyLocalInteraction();
    wl_ack_cmd2(WL_CMD_POWER, rx[2]);
}

static void wl_handle_temp_up(const uint8_t *rx, uint16_t len)
{
    uint16_t step;
    uint8_t h, t, l;

    if ((len < 7u) || (rx[2] != 0x01u)) {
        return;
    }

    step = rx[3];
    system_data.InTempSet = (uint16_t)(system_data.InTempSet + step);
    if (system_data.InTempSet > WL_TEMP_MAX) { system_data.InTempSet = WL_TEMP_MAX; }
    if (system_data.InTempSet < WL_TEMP_MIN) { system_data.InTempSet = WL_TEMP_MIN; }

    wl_split3(system_data.InTempSet, &h, &t, &l);
    s_tx[0] = WL_HEAD; s_tx[1] = WL_CMD_TEMP_UP; s_tx[2] = 0x01u;
    s_tx[3] = h; s_tx[4] = t; s_tx[5] = l; s_tx[6] = WL_TAIL;
    wl_send(s_tx, 7u);
}

static void wl_handle_temp_down(const uint8_t *rx, uint16_t len)
{
    uint16_t step;
    uint8_t h, t, l;

    if ((len < 7u) || (rx[2] != 0x01u)) {
        return;
    }

    step = rx[3];
    if (system_data.InTempSet > step) { system_data.InTempSet = (uint16_t)(system_data.InTempSet - step); }
    else { system_data.InTempSet = WL_TEMP_MIN; }
    if (system_data.InTempSet > WL_TEMP_MAX) { system_data.InTempSet = WL_TEMP_MAX; }
    if (system_data.InTempSet < WL_TEMP_MIN) { system_data.InTempSet = WL_TEMP_MIN; }

    wl_split3(system_data.InTempSet, &h, &t, &l);
    s_tx[0] = WL_HEAD; s_tx[1] = WL_CMD_TEMP_DOWN; s_tx[2] = 0x01u;
    s_tx[3] = h; s_tx[4] = t; s_tx[5] = l; s_tx[6] = WL_TAIL;
    wl_send(s_tx, 7u);
}

static void wl_handle_set_temp(const uint8_t *rx, uint16_t len)
{
    uint16_t v;

    if (len < 7u) {
        return;
    }

    if (rx[2] == 0x01u) {
        v = (uint16_t)(rx[3] * 100u + rx[4] * 10u + rx[5]);
        if (v > WL_TEMP_MAX) { v = WL_TEMP_MAX; }
        if (v < WL_TEMP_MIN) { v = WL_TEMP_MIN; }
        system_data.InTempSet = v;
    }

    s_tx[0] = WL_HEAD; s_tx[1] = WL_CMD_SET_TEMP;
    s_tx[2] = rx[2]; s_tx[3] = rx[3]; s_tx[4] = rx[4]; s_tx[5] = rx[5]; s_tx[6] = WL_TAIL;
    wl_send(s_tx, 7u);
}

static void wl_handle_jump_set(const uint8_t *rx, uint16_t len)
{
    uint8_t h, t, l;
    (void)len;

    wl_split3(system_data.InTempSet, &h, &t, &l);
    s_tx[0] = WL_HEAD; s_tx[1] = WL_CMD_JUMP_SET; s_tx[2] = rx[2];
    s_tx[3] = h; s_tx[4] = t; s_tx[5] = l; s_tx[6] = WL_TAIL;
    wl_send(s_tx, 7u);
}

static void wl_handle_query_probe(const uint8_t *rx, uint16_t len)
{
    uint16_t p;
    uint8_t h, t, l;
    (void)len;

    p = wl_get_temp_by_channel(rx[2]);
    if (p == HaveTempErr) { h = 0x09u; t = 0x06u; l = 0x00u; }
    else { wl_split3(p, &h, &t, &l); }

    s_tx[0] = WL_HEAD; s_tx[1] = WL_CMD_QUERY_PROBE; s_tx[2] = rx[2];
    s_tx[3] = 0x00u; s_tx[4] = 0x00u; s_tx[5] = 0x00u;
    s_tx[6] = h; s_tx[7] = t; s_tx[8] = l; s_tx[9] = WL_TAIL;
    wl_send(s_tx, 10u);
}

static void wl_handle_unit(const uint8_t *rx, uint16_t len)
{
    (void)len;
    if (rx[2] == 0x01u) { system_data.units = unitF; }
    else if (rx[2] == 0x02u) { system_data.units = unitC; }
    UI_NotifyLocalInteraction();
    wl_ack_cmd2(WL_CMD_UNIT, rx[2]);
}

static void wl_handle_query_status(const uint8_t *rx, uint16_t len)
{
    uint16_t in_t;
    uint16_t p1;
    uint8_t h, t, l;
    (void)len;

    if (rx[2] != 0x01u) {
        return;
    }

    s_tx[0] = WL_HEAD; s_tx[1] = 0x1Bu;
    s_tx[2] = (work_process == set) ? 0x01u : 0x02u;
    s_tx[3] = ((work_process == idle) || (work_process == shutdown)) ? 0x02u : 0x01u;
    s_tx[4] = 0x00u; s_tx[5] = 0x00u; s_tx[6] = 0x00u;
    s_tx[7] = 0x00u; s_tx[8] = 0x00u; s_tx[9] = 0x00u; s_tx[10] = 0x00u; s_tx[11] = 0x00u;
    wl_split3(system_data.InTempSet, &h, &t, &l);
    s_tx[12] = h; s_tx[13] = t; s_tx[14] = l; s_tx[15] = WL_TAIL;
    wl_send(s_tx, 16u);

    in_t = wl_get_display_main_temp();
    p1 = wl_get_probe_temp();
    s_tx[0] = WL_HEAD; s_tx[1] = 0x2Bu;
    s_tx[2] = 0x01u;
    s_tx[3] = (system_data.units == unitF) ? 0x01u : 0x02u;
    s_tx[4] = 0x00u; s_tx[5] = 0x00u; s_tx[6] = 0x00u; s_tx[7] = 0x00u;
    wl_split3(in_t, &h, &t, &l); s_tx[8] = h; s_tx[9] = t; s_tx[10] = l;
    if (p1 == HaveTempErr) { s_tx[11] = 0x09u; s_tx[12] = 0x06u; s_tx[13] = 0x00u; s_tx[14] = 0x02u; }
    else { wl_split3(p1, &h, &t, &l); s_tx[11] = h; s_tx[12] = t; s_tx[13] = l; s_tx[14] = 0x00u; }
    s_tx[15] = WL_TAIL;
    wl_send(s_tx, 16u);

    s_tx[0] = WL_HEAD; s_tx[1] = 0x3Bu;
    s_tx[2] = 0x09u; s_tx[3] = 0x06u; s_tx[4] = 0x00u; /* probe2 unsupported */
    s_tx[5] = 0x00u; s_tx[6] = 0x00u; s_tx[7] = 0x00u; s_tx[8] = 0x00u;
    s_tx[9] = 0x00u; s_tx[10] = 0x00u; s_tx[11] = 0x00u;
    s_tx[12] = 0x00u; s_tx[13] = 0x00u; s_tx[14] = 0x00u;
    s_tx[15] = WL_TAIL;
    wl_send(s_tx, 16u);
}

static void wl_handle_query_set_all(const uint8_t *rx, uint16_t len)
{
    uint8_t h, t, l;
    (void)len;
    if (rx[2] != 0x01u) { return; }

    wl_split3(system_data.InTempSet, &h, &t, &l);
    memset(s_tx, 0, 24u);
    s_tx[0] = WL_HEAD; s_tx[1] = WL_CMD_QUERY_SET_ALL;
    s_tx[20] = h; s_tx[21] = t; s_tx[22] = l; s_tx[23] = WL_TAIL;
    wl_send(s_tx, 24u);
}

static void wl_handle_query_act_all(const uint8_t *rx, uint16_t len)
{
    uint16_t in_t, p1;
    uint8_t h, t, l;
    (void)len;
    if (rx[2] != 0x01u) { return; }

    in_t = wl_get_display_main_temp();
    p1 = wl_get_probe_temp();

    s_tx[0] = WL_HEAD; s_tx[1] = WL_CMD_QUERY_ACT_ALL;
    if (p1 == HaveTempErr) { s_tx[2] = 0x09u; s_tx[3] = 0x06u; s_tx[4] = 0x00u; }
    else { wl_split3(p1, &h, &t, &l); s_tx[2] = h; s_tx[3] = t; s_tx[4] = l; }
    s_tx[5] = 0x09u; s_tx[6] = 0x06u; s_tx[7] = 0x00u;
    s_tx[8] = 0x09u; s_tx[9] = 0x06u; s_tx[10] = 0x00u;
    s_tx[11] = 0x09u; s_tx[12] = 0x06u; s_tx[13] = 0x00u;
    s_tx[14] = 0x09u; s_tx[15] = 0x06u; s_tx[16] = 0x00u;
    s_tx[17] = 0x09u; s_tx[18] = 0x06u; s_tx[19] = 0x00u;
    wl_split3(in_t, &h, &t, &l); s_tx[20] = h; s_tx[21] = t; s_tx[22] = l;
    s_tx[23] = WL_TAIL;
    wl_send(s_tx, 24u);
}

static void wl_handle_fw_info(const uint8_t *rx, uint16_t len)
{
    (void)len;
    if (rx[2] != 0x01u) { return; }

    memset(s_tx, 0, 21u);
    s_tx[0] = WL_HEAD; s_tx[1] = WL_CMD_FW_INFO; s_tx[2] = 0x01u;
    s_tx[5] = 0x01u;  /* volume */
    s_tx[6] = 0x01u;  /* type */
    s_tx[7] = 0x01u;  /* probe count */
    s_tx[10] = 0x01u; s_tx[11] = 0x07u; s_tx[12] = 0x05u; /* grill min 175 */
    s_tx[13] = 0x04u; s_tx[14] = 0x05u; s_tx[15] = 0x05u; /* grill max 455 */
    s_tx[20] = WL_TAIL;
    wl_send(s_tx, 21u);
}

static void wl_dispatch(const uint8_t *rx, uint16_t len)
{
    switch (rx[1])
    {
        case WL_CMD_POWER:         wl_handle_power(rx, len); break;
        case WL_CMD_TEMP_UP:       wl_handle_temp_up(rx, len); break;
        case WL_CMD_TEMP_DOWN:     wl_handle_temp_down(rx, len); break;
        case WL_CMD_SET_TEMP:      wl_handle_set_temp(rx, len); break;
        case WL_CMD_JUMP_SET:      wl_handle_jump_set(rx, len); break;
        case WL_CMD_QUERY_PROBE:   wl_handle_query_probe(rx, len); break;
        case WL_CMD_UNIT:          wl_handle_unit(rx, len); break;
        case WL_CMD_QUERY_STATUS:  wl_handle_query_status(rx, len); break;
        case WL_CMD_QUERY_SET_ALL: wl_handle_query_set_all(rx, len); break;
        case WL_CMD_QUERY_ACT_ALL: wl_handle_query_act_all(rx, len); break;
        case WL_CMD_BT_STATUS:
            s_bt_status = rx[2];
            if (rx[2] == 0x01u) {
                UI_SetBluetoothConnectionState(1u);
            } else {
                UI_SetBluetoothConnectionState(0u);
            }
            break;
        case WL_CMD_FW_INFO:       wl_handle_fw_info(rx, len); break;
        default:                   wl_ack_cmd2(rx[1], rx[2]); break;
    }
}

void Wireless_Init(void)
{
    COM1.rxFlag = 0u;
    COM1.rxLen = 0u;
    s_bt_status = 0u;
	
}

void WirelessTask(void)
{
    uint16_t len;
    uint8_t *rx;

    if (COM1.rxFlag == 0u) {
        return;
    }

    COM1.rxFlag = 0u;
    len = COM1.rxLen;
    if ((len < 4u) || (len > COM_RXSIZE)) {
        return;
    }

    rx = COM1.rxBuf;
    if ((rx[0] != WL_HEAD) || (rx[len - 1u] != WL_TAIL)) {
        return;
    }

    wl_dispatch(rx, len);
}
