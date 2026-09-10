#include "test_support.h"
#include "../../BSP/wireless.c"

static uint16_t CommandFrame(uint8_t *frame, const uint8_t *payload, uint8_t length)
{
    uint16_t crc;
    frame[0] = 0x5Au; frame[1] = 0x5Au; frame[2] = MATHIS_TYPE_CMD;
    frame[3] = 9u; frame[4] = length; frame[5] = 0u;
    memcpy(frame + 6, payload, length);
    crc = Mathis_Crc16(frame + 2, length + 4u);
    frame[6u + length] = (uint8_t)crc; frame[7u + length] = (uint8_t)(crc >> 8);
    return length + 8u;
}
void TestWireless(void)
{
    uint8_t frame[32], payload[2] = {MATHIS_CMD_TELEMETRY_CTRL, MATHIS_TELEMETRY_START};
    uint8_t preserved[20], status = 3u;
    uint16_t length = CommandFrame(frame, payload, sizeof(payload));
    unsigned cut, i;
    for (cut = 0u; cut <= length; cut++) {
        Wireless_Init();
        for (i = 0u; i < cut; i++) { FeedProtocolByte(frame[i]); }
        for (i = cut; i < length; i++) { FeedProtocolByte(frame[i]); }
        CHECK(g_mathis_ble_debug.frames_ok == 1u && s_connected == 1u);
    }
    Wireless_Init(); frame[length - 1u] ^= 1u;
    for (i = 0u; i < length; i++) { FeedProtocolByte(frame[i]); }
    frame[length - 1u] ^= 1u;
    for (cut = 0u; cut < 3u; cut++) { for (i = 0u; i < length; i++) { FeedProtocolByte(frame[i]); } }
    CHECK(g_mathis_ble_debug.crc_errors >= 1u && g_mathis_ble_debug.frames_ok == 3u);
    CHECK(Wireless_QueueProtocolFrame(0x90u, 0, 1u) == 0u);
    CHECK(HandleFrame(frame, 3u) == 0u);
    frame[4] = 255u; CHECK(HandleFrame(frame, sizeof(frame)) == 0u);
    Wireless_DiscardQueuedFrames(); huart1.gState = HAL_UART_STATE_READY;
    CHECK(Wireless_QueueProtocolFrame(0x90u, &status, 1u) == 1u);
    ServiceTransmit(); memcpy(preserved, mock_tx_data, mock_tx_length);
    for (i = 0u; i < 4u; i++) { status++; CHECK(Wireless_QueueProtocolFrame(0x90u, &status, 1u) == 1u); }
    CHECK(Wireless_QueueProtocolFrame(0x90u, &status, 1u) == 0u);
    CHECK(memcmp(preserved, mock_tx_data, mock_tx_length) == 0);
    Wireless_DiscardQueuedFrames(); status++; Wireless_QueueProtocolFrame(0x90u, &status, 1u);
    CHECK(memcmp(preserved, mock_tx_data, mock_tx_length) == 0);
    huart1.gState = HAL_UART_STATE_READY; ServiceTransmit();
    CHECK(mock_tx_data[6] == status);
    Wireless_DiscardQueuedFrames(); huart1.gState = HAL_UART_STATE_READY;
}
