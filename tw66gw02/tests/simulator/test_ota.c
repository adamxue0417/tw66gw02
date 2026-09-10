#include "test_support.h"
#include "ota_layout.h"
__align(1024) static uint8_t ota_flash[0x7400];
#undef OTA_STAGE_BASE
#undef OTA_SCRATCH_BASE
#undef OTA_METADATA_BASE
#define OTA_STAGE_BASE ((uint32_t)ota_flash)
#define OTA_SCRATCH_BASE ((uint32_t)ota_flash + 0x6C00u)
#define OTA_METADATA_BASE ((uint32_t)ota_flash + 0x7000u)
#include "../../COP/ota_update.c"

void TestOta(void)
{
    uint8_t begin[9], chunk[8], commit[4];
    uint32_t crc;
    unsigned writes;
    CHECK(OtaUpdate_HandleFrame(0x10u, 0, 9u) == 0u);
    WriteU32Le(begin, 640u); WriteU32Le(begin + 4, 0u); begin[8] = 101u;
    g_battery_mv = 1500u; g_battery_sample_valid = 0u;
    OtaUpdate_Init(); HandleBegin(begin, sizeof(begin)); CHECK(s_ota.state == OTA_STATE_IDLE);
    g_battery_sample_valid = 1u;
    HandleBegin(begin, sizeof(begin)); CHECK(s_ota.state == OTA_STATE_READY);
    WriteU32Le(chunk, 0u); chunk[4] = 1u; chunk[5] = 2u; chunk[6] = 3u; chunk[7] = 4u;
    writes = mock_flash_writes; HandleChunk(chunk, 7u); CHECK(mock_flash_writes == writes);
    HandleChunk(chunk, 8u); CHECK(s_ota.next_offset == 4u);
    writes = mock_flash_writes; HandleChunk(chunk, 8u); CHECK(mock_flash_writes == writes);
    WriteU32Le(chunk, 0xFFFFFFFEu); HandleChunk(chunk, 8u); CHECK(s_ota.next_offset == 4u);
    CHECK(ProgramBytes(OTA_SCRATCH_BASE - 1u, chunk, 2u) == 0u);
    CHECK(EraseRange(OTA_SCRATCH_BASE, 1u) == 0u);
    s_ota.timeout_ticks = OTA_TIMEOUT_TICKS_20MS - 1u; OtaUpdate_Task20ms(); CHECK(s_ota.state == OTA_STATE_IDLE);
    HandleBegin(begin, sizeof(begin));
    *(uint32_t *)ota_flash = OTA_APP_RAM_END;
    *(uint32_t *)(ota_flash + 4) = OTA_APP_BASE + 193u;
    CHECK(ApplicationVectorValid(256u) == 1u);
    *(uint32_t *)(ota_flash + 4) &= ~1u; CHECK(ApplicationVectorValid(256u) == 0u);
    *(uint32_t *)(ota_flash + 4) |= 1u;
    s_ota.next_offset = s_ota.artifact_size; s_ota.state = OTA_STATE_TRANSFERRING;
    crc = FlashCrc32(OTA_STAGE_BASE, s_ota.artifact_size);
    s_ota.expected_crc = crc; WriteU32Le(commit, crc);
    mock_flash_fail = 2; HandleCommit(commit, sizeof(commit)); CHECK(s_ota.state == OTA_STATE_IDLE);
    mock_flash_fail = 0;
}
