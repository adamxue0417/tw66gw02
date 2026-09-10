exec(open('outputs/optimization/edit_firmware.py',encoding='utf-8').read().split("s=read('Core/Src/main.c')")[0])

s=read('BSP/temp.c').replace('        adc_value[i] = 0u;','        /* DMA owns adc_value while continuous conversion is running. */')
write('BSP/temp.c',s)
s=read('BSP/wireless.c').replace('#include "ota_update.h"','#include "ota_update.h"\n#include "mathis_util.h"')
s=s.replace('static uint8_t s_tx_count;','static uint8_t s_tx_count;\n/* DMA retains this storage until HAL reports completion, even after queue reset. */\nstatic uint8_t s_tx_inflight[20];')
s=function(s,'Mathis_Crc16','''uint16_t Mathis_Crc16(const uint8_t *data, uint16_t length)
{
    if ((data == 0) && (length != 0u)) { return 0u; }
    return Mathis_Crc16Bytes(data, length);
}''')
s=function(s,'HandleFrame','''static uint8_t HandleFrame(const uint8_t *frame, uint16_t length)
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
}''')
# Parser only removes bytes already consumed, preserving a trailing start byte.
s=function(s,'ResyncBufferedFrame','''static void ConsumeRx(uint16_t count)
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
}''')
s=function(s,'FeedProtocolByte','''static void FeedProtocolByte(uint8_t byte)
{
    if (s_rx_count >= sizeof(s_rx_frame)) { ConsumeRx(1u); }
    s_rx_frame[s_rx_count++] = byte;
    ResyncBufferedFrame();
}''')
s=s.replace('if ((length > sizeof(s_tx_queue[0].data)) || (s_tx_count >= TX_QUEUE_DEPTH))', 'if ((data == 0) || (length == 0u) || (length > sizeof(s_tx_queue[0].data)) || (s_tx_count >= TX_QUEUE_DEPTH))')
s=s.replace('if (payload_length > 12u)', 'if ((payload_length > 12u) || ((payload_length != 0u) && (payload == 0)))')
s=function(s,'QueueTelemetry','''static void QueueTelemetry(void)
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
}''')
s=s.replace('    if (HAL_UART_Transmit_DMA(&huart1, s_tx_queue[s_tx_head].data, s_tx_queue[s_tx_head].length) == HAL_OK)', '    memcpy(s_tx_inflight, s_tx_queue[s_tx_head].data, s_tx_queue[s_tx_head].length);\n    if (HAL_UART_Transmit_DMA(&huart1, s_tx_inflight, s_tx_queue[s_tx_head].length) == HAL_OK)')
write('BSP/wireless.c',s)

s=read('COP/main_control.c').replace('#include "ota_layout.h"','#include "ota_layout.h"\n#include "mathis_util.h"')
s=re.sub(r'(?m)^//.*\n','',s)  # remove the disabled historical fitting implementation
s=function(s,'ConfigCrc16','''static uint16_t ConfigCrc16(const uint8_t *data, uint16_t length)
{
    return Mathis_Crc16Bytes(data, length);
}''')
s=s.replace('static uint8_t ConfigRecordValid(const PersistedConfig *cfg)\n{','''static uint8_t ConfigRecordValid(const PersistedConfig *cfg)
{
    uint8_t channel, term;
    for (channel = 0u; channel < TEMP_COEFF_CHANNEL_COUNT; channel++) {
        for (term = 0u; term < TEMP_COEFF_TERM_COUNT; term++) {
            if (Mathis_FloatFinite(cfg->coeff[channel][term]) == 0u) { return 0u; }
        }
    }''')
s=s.replace('    memset(&cfg, 0xFF, sizeof(cfg));','''    if (s_config_active_page != 0u) {
        const PersistedConfig *active = (const PersistedConfig *)s_config_active_page;
        if ((ConfigRecordValid(active) != 0u) && (active->units == system_data.units) &&
            (memcmp(active->coeff, s_temp_coeff, sizeof(s_temp_coeff)) == 0)) { return 1u; }
    }
    memset(&cfg, 0xFF, sizeof(cfg));''')
s=s.replace('    HAL_FLASH_Lock(); s_config_sequence = cfg.sequence; s_config_active_page = target;', '''    HAL_FLASH_Lock();
    if ((ConfigRecordValid((const PersistedConfig *)target) == 0u) ||
        (memcmp((const void *)target, &cfg, sizeof(cfg)) != 0)) { return 0u; }
    s_config_sequence = cfg.sequence; s_config_active_page = target;''')
s=s.replace('    if (v > 32767.0f)', '    if (Mathis_FloatFinite(v) == 0u) { return HaveTempErr; }\n    if (v > 32767.0f)')
s=s.replace('    if (y > 500.0f)', '    if (Mathis_FloatFinite(y) == 0u) { return HaveTempErr; }\n    if (y > 500.0f)')
s=s.replace('static uint32_t s_config_sequence = 0u;', '''static int16_t s_adjusted_raw[TEMP_COEFF_CHANNEL_COUNT];
static int16_t s_adjusted_value[TEMP_COEFF_CHANNEL_COUNT];
static uint8_t s_adjusted_valid[TEMP_COEFF_CHANNEL_COUNT];
static uint32_t s_config_sequence = 0u;''')
s=function(s,'UI_GetAdjustedMainTemp','''int16_t UI_GetAdjustedMainTemp(uint8_t src_idx)
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
}''')
s=s.replace('        previous[i] = s_temp_coeff[src_idx][i];','        if (Mathis_FloatFinite(coeff[i]) == 0u) { return 0u; }\n    }\n    for (i = 0u; i < TEMP_COEFF_TERM_COUNT; i++) {\n        previous[i] = s_temp_coeff[src_idx][i];',1)
s=s.replace('    if (ConfigSave() == 0u) {\n        for', '    s_adjusted_valid[src_idx] = 0u;\n    if (ConfigSave() == 0u) {\n        for',1)
s=s.replace('adjusted = ApplyMainTempCoefficient(internal_index[i], raw);','adjusted = UI_GetAdjustedMainTemp(internal_index[i]);\n        if (adjusted == HaveTempErr) { snapshot->error_code = 0x02u; continue; }')
s=re.sub(r'ApplyMainTempCoefficient\(([012])u, ch_temp\)',r'UI_GetAdjustedMainTemp(\1u)',s)
s=function(s,'UI_FactoryReset','''void UI_FactoryReset(void)
{
    uint8_t previous = system_data.units;
    system_data.units = unitC;
    if (ConfigSave() == 0u) { system_data.units = previous; }
    UI_NotifyLocalInteraction();
}''')
write('COP/main_control.c',s)

for name in ['COP/ota_update.c','Bootloader/boot_main.c']:
    s=read(name).replace('#include "ota_layout.h"','#include "ota_layout.h"\n#include "mathis_util.h"')
    s=function(s,'Crc32Update','''static uint32_t Crc32Update(uint32_t crc, const uint8_t *data, uint32_t length)
{
    return Mathis_Crc32Update(crc, data, length);
}''')
    if name.startswith('COP'):
        s=s.replace('    payload[0] = status;','    if ((extra_length > 4u) || ((extra_length != 0u) && (extra == 0))) { return 0u; }\n    payload[0] = status;')
        s=s.replace('    erase.TypeErase = FLASH_TYPEERASE_PAGES;','''    if ((pages == 0u) || (pages > OTA_SLOT_PAGE_COUNT) ||
        ((address & (OTA_PAGE_SIZE - 1u)) != 0u) ||
        (Mathis_RangeWithin(address, pages * OTA_PAGE_SIZE, OTA_STAGE_BASE, OTA_SCRATCH_BASE) == 0u &&
         !((address == OTA_METADATA_BASE) && (pages == 1u)))) { return 0u; }
    erase.TypeErase = FLASH_TYPEERASE_PAGES;''',1)
        s=s.replace('    if ((address & 1u) != 0u) { return 0u; }','''    if ((data == 0) || (length == 0u) || ((address & 1u) != 0u) ||
        (Mathis_RangeWithin(address, ((uint32_t)length + 1u) & ~1u, OTA_STAGE_BASE, OTA_SCRATCH_BASE) == 0u)) { return 0u; }''',1)
        s=function(s,'ApplicationVectorValid','''static uint8_t ApplicationVectorValid(uint32_t application_size)
{
    return Mathis_AppVectorValid(*(const uint32_t *)OTA_STAGE_BASE,
                                *(const uint32_t *)(OTA_STAGE_BASE + 4u), application_size);
}''')
        s=s.replace('if (snapshot.battery_percent < 30u)', 'if ((g_battery_sample_valid == 0u) || (snapshot.battery_percent < 30u))')
        s=s.replace('    if ((offset != s_ota.next_offset) || ((offset & 1u) != 0u))', '    if ((offset != s_ota.next_offset) || ((offset & 1u) != 0u) ||\n        (((data_length & 1u) != 0u) && (offset + data_length != s_ota.artifact_size)))')
        s=s.replace('    switch (type) {', '    if ((length != 0u) && (payload == 0)) { return 0u; }\n    switch (type) {',1)
    else:
        s=function(s,'ImageVectorValid','''static uint8_t ImageVectorValid(uint32_t address, uint32_t length)
{
    return Mathis_AppVectorValid(*(const uint32_t *)address,
                                *(const uint32_t *)(address + 4u), length);
}''')
        s=s.replace('    if (FlashErasePage(destination) == 0u)', '    if ((valid_bytes > OTA_PAGE_SIZE) || ((source & (OTA_PAGE_SIZE - 1u)) != 0u) ||\n        (Mathis_RangeWithin(source, OTA_PAGE_SIZE, OTA_APP_BASE, OTA_METADATA_BASE) == 0u)) { return 0u; }\n    if (FlashErasePage(destination) == 0u)')
        s=s.replace('(offset < valid_bytes) ? *(const uint16_t *)(source + offset) : 0xFFFFu','Mathis_PageHalfword((const uint8_t *)source, offset, valid_bytes)')
    write(name,s)
