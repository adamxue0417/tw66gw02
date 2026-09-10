/** Mathis BLE framed protocol, including the development OTA transport. */
#include "wireless.h"
#include "config.h"
#include "ota_update.h"
#include "mathis_util.h"

#define TX_QUEUE_DEPTH              (4u)
#define TELEMETRY_PERIOD_TICKS_20MS (50u)
#define COEFF_TIMEOUT_TICKS_20MS    (3000u)
#define FW_RELEASE_NUMBER           ((uint8_t)system_version)

typedef struct {
    uint8_t data[20];
    uint8_t length;
} TxEntry;

typedef struct {
    float coeff[TEMP_COEFF_TERM_COUNT];
    uint8_t parts;
    uint16_t age;
} CoeffPending;

static uint8_t s_rx_frame[MATHIS_MAX_FRAME];
static uint8_t s_uart_chunk[COM_RXSIZE];
static uint16_t s_rx_count;
static uint16_t s_rx_expected;
static uint8_t s_connected;
static uint8_t s_tx_sequence;
static uint8_t s_telemetry_ticks;
static TxEntry s_tx_queue[TX_QUEUE_DEPTH];
static uint8_t s_tx_head;
static uint8_t s_tx_tail;
static uint8_t s_tx_count;
/* DMA retains this storage until HAL reports completion, even after queue reset. */
static uint8_t s_tx_inflight[20];
static CoeffPending s_pending[TEMP_COEFF_CHANNEL_COUNT];
MathisBleDebug g_mathis_ble_debug;

static void SetConnection(uint8_t connected);
static uint8_t QueueFrame(const uint8_t *data, uint8_t length);

uint16_t Mathis_Crc16(const uint8_t *data, uint16_t length)
{
    if ((data == 0) && (length != 0u)) { return 0u; }
    return Mathis_Crc16Bytes(data, length);
}

static uint8_t Float32Valid(const uint8_t *bytes)
{
    uint32_t raw;
    memcpy(&raw, bytes, sizeof(raw));
    return ((raw & 0x7F800000u) == 0x7F800000u) ? 0u : 1u;
}

static float ReadFloatLe(const uint8_t *bytes)
{
    float value;
    memcpy(&value, bytes, sizeof(value));
    return value;
}

static void ClearPending(void)
{
    memset(s_pending, 0, sizeof(s_pending));
}

static uint8_t ProtocolToInternalChannel(uint8_t channel)
{
    static const uint8_t map[3] = {2u, 1u, 0u};
    return (channel < 3u) ? map[channel] : 0xFFu;
}

static uint8_t HandleCoefficientPart(uint8_t subcmd, const uint8_t *payload, uint16_t length)
{
    uint8_t protocol_channel;
    uint8_t internal_channel;
    CoeffPending *pending;
    if (((subcmd == MATHIS_CMD_COEFF_E) && (length != 7u)) ||
        ((subcmd != MATHIS_CMD_COEFF_E) && (length != 11u))) { return 0u; }
    if (payload[1] != MATHIS_MODEL_ID) { return 0u; }
    protocol_channel = payload[2];
    internal_channel = ProtocolToInternalChannel(protocol_channel);
    if (internal_channel == 0xFFu) { return 0u; }
    if ((Float32Valid(&payload[3]) == 0u) ||
        ((subcmd != MATHIS_CMD_COEFF_E) && (Float32Valid(&payload[7]) == 0u))) { return 0u; }
    pending = &s_pending[internal_channel];
    pending->age = 0u;
    if (subcmd == MATHIS_CMD_COEFF_AB) {
        pending->coeff[0] = ReadFloatLe(&payload[3]); pending->coeff[1] = ReadFloatLe(&payload[7]); pending->parts |= 0x01u;
    } else if (subcmd == MATHIS_CMD_COEFF_CD) {
        pending->coeff[2] = ReadFloatLe(&payload[3]); pending->coeff[3] = ReadFloatLe(&payload[7]); pending->parts |= 0x02u;
    } else {
        pending->coeff[4] = ReadFloatLe(&payload[3]); pending->parts |= 0x04u;
    }
    if (pending->parts == 0x07u) {
        (void)UI_SetTempCoeff(internal_channel, pending->coeff);
        memset(pending, 0, sizeof(*pending));
    }
    return 1u;
}

static void HandleCommand(const uint8_t *payload, uint16_t length)
{
    uint8_t command;
    if (length == 0u) { return; }
    command = payload[0];
    g_mathis_ble_debug.cmd_frames++;
    g_mathis_ble_debug.last_command = command;
    switch (command) {
        case MATHIS_CMD_SET_UNITS:
            if ((length == 2u) && ((payload[1] == unitC) || (payload[1] == unitF))) {
                (void)UI_SetUnits(payload[1], 1u); UI_NotifyLocalInteraction();
            }
            break;
        case MATHIS_CMD_POWER_OFF:
            if (length == 1u) { UI_NotifyLocalInteraction(); work_process = shutdown; }
            break;
        case MATHIS_CMD_FACTORY_RESET:
            if (length == 1u) {
                UI_FactoryReset(); ClearPending(); SetConnection(0u); UI_RequestBluetoothPowerOff();
            }
            break;
        case MATHIS_CMD_COEFF_AB:
        case MATHIS_CMD_COEFF_CD:
        case MATHIS_CMD_COEFF_E:
            if (HandleCoefficientPart(command, payload, length) != 0u) { UI_NotifyLocalInteraction(); }
            break;
        case MATHIS_CMD_TELEMETRY_CTRL:
            if ((length == 2u) && (payload[1] == MATHIS_TELEMETRY_START)) {
                g_mathis_ble_debug.start_frames++;
                SetConnection(1u);
            } else if ((length == 2u) && (payload[1] == MATHIS_TELEMETRY_STOP)) {
                g_mathis_ble_debug.stop_frames++;
                SetConnection(0u);
            }
            break;
        default:
            break;
    }
}

static uint8_t HandleFrame(const uint8_t *frame, uint16_t length)
{
    uint16_t payload_length, received_crc;
    if ((frame == 0) || (length < 8u)) { return 0u; }
    payload_length = (uint16_t)frame[4] | ((uint16_t)frame[5] << 8);
    if ((payload_length > MATHIS_MAX_PAYLOAD) || (length != payload_length + 8u)) {
        g_mathis_ble_debug.length_errors++;
        return 0u;
    }
    received_crc = (uint16_t)frame[6u + payload_length] | ((uint16_t)frame[7u + payload_length] << 8);
    g_mathis_ble_debug.last_type = frame[2];
    g_mathis_ble_debug.last_sequence = frame[3];
    g_mathis_ble_debug.last_payload_length = payload_length;
    if (received_crc != Mathis_Crc16(&frame[2], 4u + payload_length)) {
        g_mathis_ble_debug.crc_errors++;
        return 0u;
    }
    g_mathis_ble_debug.frames_ok++;
    if (frame[2] == MATHIS_TYPE_CMD) { HandleCommand(&frame[6], payload_length); }
    else { (void)OtaUpdate_HandleFrame(frame[2], &frame[6], payload_length); }
    return 1u;
}

static void ConsumeRx(uint16_t count)
{
    if (count >= s_rx_count) { s_rx_count = 0u; }
    else {
        s_rx_count -= count;
        memmove(s_rx_frame, s_rx_frame + count, s_rx_count);
    }
    s_rx_expected = 0u;
}

static void ResyncBufferedFrame(void)
{
    while (s_rx_count != 0u) {
        uint16_t payload_length;
        if (s_rx_frame[0] != MATHIS_START_BYTE) { ConsumeRx(1u); continue; }
        if (s_rx_count == 1u) { return; }
        if (s_rx_frame[1] != MATHIS_START_BYTE) { ConsumeRx(1u); continue; }
        if (s_rx_count < 6u) { return; }
        payload_length = (uint16_t)s_rx_frame[4] | ((uint16_t)s_rx_frame[5] << 8);
        if (payload_length > MATHIS_MAX_PAYLOAD) { ConsumeRx(1u); continue; }
        s_rx_expected = payload_length + 8u;
        if (s_rx_count < s_rx_expected) { return; }
        {
            uint16_t frame_length = s_rx_expected;
            /* A disconnect command may clear the parser through SetConnection. */
            if (HandleFrame(s_rx_frame, frame_length) != 0u) { ConsumeRx(frame_length); }
            else { ConsumeRx(1u); }
        }
    }
}

static void FeedProtocolByte(uint8_t byte)
{
    if (s_rx_count >= sizeof(s_rx_frame)) { ConsumeRx(1u); }
    s_rx_frame[s_rx_count++] = byte;
    ResyncBufferedFrame();
}

static void SetConnection(uint8_t connected)
{
    connected = (connected != 0u) ? 1u : 0u;
    if (s_connected != connected) {
        s_connected = connected; UI_SetBluetoothConnectionState(connected);
        g_mathis_ble_debug.telemetry_enabled = connected;
        s_telemetry_ticks = 0u;
        if (connected == 0u) {
            ClearPending(); s_tx_head = 0u; s_tx_tail = 0u; s_tx_count = 0u;
            s_rx_count = 0u; s_rx_expected = 0u;
            OtaUpdate_HandleDisconnect();
        }
    }
}

static void FeedIncomingByte(uint8_t byte)
{
    /* FF 01 / FF 00 are accepted only as payload of a CRC-valid Mathis CMD. */
    FeedProtocolByte(byte);
}

static uint8_t HandleModuleTelemetryEvent(const uint8_t *data, uint16_t length)
{
    /*
     * Some EMB1082 firmware revisions report TX-notify subscription state as
     * a standalone two-byte UART event. This is a module-local transport event,
     * not an App Mathis message. Only recognise the exact two-byte DMA chunk
     * while the framed parser is idle, so FF 01/FF 00 inside a split Mathis
     * payload cannot be mistaken for a connection event.
     */
    if ((s_rx_count != 0u) || (length != 2u) || (data[0] != MATHIS_CMD_TELEMETRY_CTRL)) {
        return 0u;
    }
    if (data[1] == MATHIS_TELEMETRY_START) {
        g_mathis_ble_debug.module_start_events++;
        SetConnection(1u);
        return 1u;
    }
    if (data[1] == MATHIS_TELEMETRY_STOP) {
        g_mathis_ble_debug.module_stop_events++;
        SetConnection(0u);
        return 1u;
    }
    return 0u;
}

static uint8_t QueueFrame(const uint8_t *data, uint8_t length)
{
    if ((data == 0) || (length == 0u) || (length > sizeof(s_tx_queue[0].data)) || (s_tx_count >= TX_QUEUE_DEPTH)) { return 0u; }
    memcpy(s_tx_queue[s_tx_tail].data, data, length); s_tx_queue[s_tx_tail].length = length;
    s_tx_tail = (uint8_t)((s_tx_tail + 1u) % TX_QUEUE_DEPTH); s_tx_count++;
    return 1u;
}

uint8_t Wireless_QueueProtocolFrame(uint8_t type, const uint8_t *payload, uint8_t payload_length)
{
    uint8_t frame[20];
    uint16_t crc;
    uint8_t frame_length;
    if ((payload_length > 12u) || ((payload_length != 0u) && (payload == 0))) { return 0u; }
    frame[0] = MATHIS_START_BYTE; frame[1] = MATHIS_START_BYTE;
    frame[2] = type; frame[3] = s_tx_sequence;
    frame[4] = payload_length; frame[5] = 0u;
    if ((payload_length != 0u) && (payload != 0)) { memcpy(&frame[6], payload, payload_length); }
    crc = Mathis_Crc16(&frame[2], (uint16_t)(4u + payload_length));
    frame[6u + payload_length] = (uint8_t)crc;
    frame[7u + payload_length] = (uint8_t)(crc >> 8);
    frame_length = (uint8_t)(payload_length + 8u);
    if (QueueFrame(frame, frame_length) == 0u) { return 0u; }
    s_tx_sequence++;
    return 1u;
}

uint8_t Wireless_ProtocolTxIdle(void)
{
    return ((s_tx_count == 0u) && (huart1.gState == HAL_UART_STATE_READY)) ? 1u : 0u;
}

void Wireless_DiscardQueuedFrames(void)
{
    s_tx_head = 0u; s_tx_tail = 0u; s_tx_count = 0u;
}

static int16_t EncodeTemperature(const MathisTelemetrySnapshot *snapshot, uint8_t channel)
{
    uint8_t mask = (uint8_t)(1u << channel);
    if ((snapshot->high_mask & mask) != 0u) { return 2000; }
    if ((snapshot->low_mask & mask) != 0u) { return 3000; }
    if ((snapshot->valid_mask & mask) == 0u) { return 4000; }
    /* Mathis telemetry is always encoded in whole degrees Celsius. */
    return snapshot->temp_c[channel];
}

static void QueueTelemetry(void)
{
    MathisTelemetrySnapshot snapshot;
    uint8_t payload[12];
    uint8_t channel;
    UI_GetTelemetrySnapshot(&snapshot);
    payload[0] = (uint8_t)(snapshot.valid_mask & 0x0Fu);
    if (snapshot.units == unitC) { payload[0] |= 0x10u; }
    for (channel = 0u; channel < 4u; channel++) {
        int16_t value = EncodeTemperature(&snapshot, channel);
        payload[1u + channel * 2u] = (uint8_t)value;
        payload[2u + channel * 2u] = (uint8_t)((uint16_t)value >> 8);
    }
    payload[9] = snapshot.battery_percent;
    payload[10] = FW_RELEASE_NUMBER;
    payload[11] = snapshot.error_code;
    if (Wireless_QueueProtocolFrame(MATHIS_TYPE_TELEMETRY, payload, sizeof(payload)) != 0u) {
        g_mathis_ble_debug.telemetry_queued++;
    }
}

static void ServiceTransmit(void)
{
    if ((s_tx_count == 0u) || (huart1.gState != HAL_UART_STATE_READY)) { return; }
    memcpy(s_tx_inflight, s_tx_queue[s_tx_head].data, s_tx_queue[s_tx_head].length);
    if (HAL_UART_Transmit_DMA(&huart1, s_tx_inflight, s_tx_queue[s_tx_head].length) == HAL_OK) {
        s_tx_head = (uint8_t)((s_tx_head + 1u) % TX_QUEUE_DEPTH); s_tx_count--;
        g_mathis_ble_debug.telemetry_tx_started++;
    }
}

void Wireless_Init(void)
{
    s_rx_count = 0u; s_rx_expected = 0u; s_connected = 0u;
    s_tx_sequence = 0u; s_telemetry_ticks = 0u; s_tx_head = 0u; s_tx_tail = 0u; s_tx_count = 0u;
    memset(&g_mathis_ble_debug, 0, sizeof(g_mathis_ble_debug));
    ClearPending(); OtaUpdate_Init(); COM1.rxFlag = 0u; COM1.rxLen = 0u;
}

void WirelessTask(void)
{
    uint16_t i;
    uint16_t length;
    uint8_t channel;
    length = COM_TakeRx(&COM1, s_uart_chunk, sizeof(s_uart_chunk));
    if (length != 0u) {
        g_mathis_ble_debug.uart_chunks++;
        g_mathis_ble_debug.uart_bytes += length;
        g_mathis_ble_debug.last_uart_length = length;
        if (HandleModuleTelemetryEvent(s_uart_chunk, length) == 0u) {
            for (i = 0u; i < length; i++) { FeedIncomingByte(s_uart_chunk[i]); }
        }
    }
    for (channel = 0u; channel < TEMP_COEFF_CHANNEL_COUNT; channel++) {
        if (s_pending[channel].parts != 0u) {
            if (++s_pending[channel].age >= COEFF_TIMEOUT_TICKS_20MS) { memset(&s_pending[channel], 0, sizeof(s_pending[channel])); }
        }
    }
    OtaUpdate_Task20ms();
    if ((s_connected != 0u) && (OtaUpdate_IsActive() == 0u)) {
        if (++s_telemetry_ticks >= TELEMETRY_PERIOD_TICKS_20MS) { s_telemetry_ticks = 0u; QueueTelemetry(); }
    }
    ServiceTransmit();
}
