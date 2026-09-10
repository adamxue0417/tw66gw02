#include "config.h"
#include "ota_layout.h"
#include "mathis_util.h"
#include "ota_boot.h"
#include "ota_update.h"

#define OTA_TYPE_BEGIN             (0x10u)
#define OTA_TYPE_CHUNK             (0x11u)
#define OTA_TYPE_COMMIT            (0x12u)
#define OTA_TYPE_ABORT             (0x13u)
#define OTA_TYPE_STATUS            (0x90u)

#define OTA_STATUS_READY           (0x01u)
#define OTA_STATUS_ACK             (0x03u)
#define OTA_STATUS_VERIFY_OK       (0x04u)
#define OTA_STATUS_VERIFY_FAILED   (0x05u)
#define OTA_STATUS_APPLYING        (0x06u)
#define OTA_STATUS_ABORTED         (0x07u)
#define OTA_STATUS_POWER           (0x08u)
#define OTA_STATUS_STORAGE         (0x09u)
#define OTA_STATUS_BUSY            (0x0Au)
#define OTA_STATUS_BAD_OFFSET      (0x0Bu)

#define OTA_REASON_CRC             (0x01u)
#define OTA_REASON_SIZE            (0x03u)
#define OTA_REASON_TIMEOUT         (0x04u)
#define OTA_REASON_UNKNOWN         (0xFFu)
#define OTA_TIMEOUT_TICKS_20MS     (3000u)
#define OTA_RESET_DELAY_TICKS      (25u)

enum
{
    OTA_STATE_IDLE = 0,
    OTA_STATE_READY,
    OTA_STATE_TRANSFERRING,
    OTA_STATE_APPLYING
};

typedef struct
{
    uint32_t artifact_size;
    uint32_t expected_crc;
    uint32_t next_offset;
    uint32_t last_offset;
    uint16_t last_length;
    uint16_t timeout_ticks;
    uint16_t reset_ticks;
    uint8_t target_version;
    uint8_t state;
} OtaSession;

static OtaSession s_ota;

static uint32_t ReadU32Le(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void WriteU32Le(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16); p[3] = (uint8_t)(value >> 24);
}

static uint32_t Crc32Update(uint32_t crc, const uint8_t *data, uint32_t length)
{
    return Mathis_Crc32Update(crc, data, length);
}

static uint32_t FlashCrc32(uint32_t address, uint32_t length)
{
    return Crc32Update(0xFFFFFFFFu, (const uint8_t *)address, length) ^ 0xFFFFFFFFu;
}

static uint8_t QueueStatus(uint8_t status, const uint8_t *extra, uint8_t extra_length)
{
    uint8_t payload[5];
    if ((extra_length > 4u) || ((extra_length != 0u) && (extra == 0))) { return 0u; }
    payload[0] = status;
    if ((extra_length != 0u) && (extra != 0)) { memcpy(&payload[1], extra, extra_length); }
    return Wireless_QueueProtocolFrame(OTA_TYPE_STATUS, payload, (uint8_t)(extra_length + 1u));
}

static void QueueReason(uint8_t status, uint8_t reason)
{
    (void)QueueStatus(status, &reason, 1u);
}

static uint8_t EraseRange(uint32_t address, uint32_t pages)
{
    FLASH_EraseInitTypeDef erase;
    uint32_t page_error = 0u;
    if ((pages == 0u) || (pages > OTA_SLOT_PAGE_COUNT) ||
        ((address & (OTA_PAGE_SIZE - 1u)) != 0u) ||
        (Mathis_RangeWithin(address, pages * OTA_PAGE_SIZE, OTA_STAGE_BASE, OTA_SCRATCH_BASE) == 0u &&
         !((address == OTA_METADATA_BASE) && (pages == 1u)))) { return 0u; }
    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = address;
    erase.NbPages = pages;
    if (HAL_FLASH_Unlock() != HAL_OK) { return 0u; }
    if (HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK) {
        (void)HAL_FLASH_Lock(); return 0u;
    }
    (void)HAL_FLASH_Lock();
    return 1u;
}

static uint8_t ProgramBytes(uint32_t address, const uint8_t *data, uint16_t length)
{
    uint16_t offset;
    uint16_t value;
    if ((data == 0) || (length == 0u) || ((address & 1u) != 0u) ||
        (Mathis_RangeWithin(address, ((uint32_t)length + 1u) & ~1u, OTA_STAGE_BASE, OTA_SCRATCH_BASE) == 0u)) { return 0u; }
    if (HAL_FLASH_Unlock() != HAL_OK) { return 0u; }
    for (offset = 0u; offset < length; offset += 2u) {
        value = data[offset];
        if ((uint16_t)(offset + 1u) < length) { value |= (uint16_t)data[offset + 1u] << 8; }
        else { value |= 0xFF00u; }
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, address + offset, value) != HAL_OK) {
            (void)HAL_FLASH_Lock(); return 0u;
        }
    }
    (void)HAL_FLASH_Lock();
    return (memcmp((const void *)address, data, length) == 0) ? 1u : 0u;
}

static uint8_t ApplicationVectorValid(uint32_t application_size)
{
    return Mathis_AppVectorValid(*(const uint32_t *)OTA_STAGE_BASE,
                                *(const uint32_t *)(OTA_STAGE_BASE + 4u), application_size);
}

static uint8_t WriteMetadata(void)
{
    OtaMetadataHeader header;
    uint16_t offset;
    const uint16_t *source = (const uint16_t *)&header;
    memset(&header, 0xFF, sizeof(header));
    header.magic = OTA_METADATA_MAGIC;
    header.format = OTA_METADATA_FORMAT;
    header.header_size = sizeof(header);
    header.artifact_size = s_ota.artifact_size;
    header.application_size = s_ota.artifact_size - OTA_SIGNATURE_SIZE;
    header.artifact_crc32 = s_ota.expected_crc;
    header.target_version = s_ota.target_version;
    header.flags = 0u;
    header.reserved0 = 0xFFFFu;
    header.header_crc32 = Crc32Update(0xFFFFFFFFu, (const uint8_t *)&header, 24u) ^ 0xFFFFFFFFu;
    header.commit = OTA_METADATA_COMMIT;
    header.reserved1 = 0xFFFFu;
    if (EraseRange(OTA_METADATA_BASE, 1u) == 0u) { return 0u; }
    if (HAL_FLASH_Unlock() != HAL_OK) { return 0u; }
    for (offset = 0u; offset < 28u; offset += 2u) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, OTA_METADATA_BASE + offset,
                              source[offset / 2u]) != HAL_OK) {
            (void)HAL_FLASH_Lock(); return 0u;
        }
    }
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, OTA_METADATA_BASE + 28u,
                          OTA_METADATA_COMMIT) != HAL_OK) {
        (void)HAL_FLASH_Lock(); return 0u;
    }
    (void)HAL_FLASH_Lock();
    return 1u;
}

static void ReturnIdle(void)
{
    memset(&s_ota, 0, sizeof(s_ota));
}

static void AbortSession(uint8_t reason)
{
    QueueReason(OTA_STATUS_ABORTED, reason);
    ReturnIdle();
}

static void HandleBegin(const uint8_t *payload, uint16_t length)
{
    MathisTelemetrySnapshot snapshot;
    uint8_t ready[2];
    uint32_t size;
    if (s_ota.state != OTA_STATE_IDLE) { (void)QueueStatus(OTA_STATUS_BUSY, 0, 0u); return; }
    if (length != 9u) { QueueReason(OTA_STATUS_VERIFY_FAILED, OTA_REASON_SIZE); return; }
    size = ReadU32Le(payload);
    if ((size <= OTA_SIGNATURE_SIZE) || (size > OTA_MAX_ARTIFACT_SIZE)) {
        QueueReason(OTA_STATUS_VERIFY_FAILED, OTA_REASON_SIZE); return;
    }
    if (OtaBoot_CanStartUpdate() == 0u) { (void)QueueStatus(OTA_STATUS_BUSY, 0, 0u); return; }
    UI_GetTelemetrySnapshot(&snapshot);
    if ((g_battery_sample_valid == 0u) || (snapshot.battery_percent < 30u)) { (void)QueueStatus(OTA_STATUS_POWER, 0, 0u); return; }
    if ((EraseRange(OTA_STAGE_BASE, OTA_SLOT_PAGE_COUNT) == 0u) ||
        (EraseRange(OTA_METADATA_BASE, 1u) == 0u)) {
        (void)QueueStatus(OTA_STATUS_STORAGE, 0, 0u); return;
    }
    Wireless_DiscardQueuedFrames();
    memset(&s_ota, 0, sizeof(s_ota));
    s_ota.artifact_size = size;
    s_ota.expected_crc = ReadU32Le(&payload[4]);
    s_ota.target_version = payload[8];
    s_ota.state = OTA_STATE_READY;
    ready[0] = (uint8_t)OTA_ACCEPTED_CHUNK_SIZE;
    ready[1] = (uint8_t)(OTA_ACCEPTED_CHUNK_SIZE >> 8);
    (void)QueueStatus(OTA_STATUS_READY, ready, 2u);
}

static void HandleChunk(const uint8_t *payload, uint16_t length)
{
    uint8_t next[4];
    uint32_t offset;
    uint16_t data_length;
    if ((s_ota.state != OTA_STATE_READY) && (s_ota.state != OTA_STATE_TRANSFERRING)) {
        (void)QueueStatus(OTA_STATUS_BUSY, 0, 0u); return;
    }
    if (length <= 4u) { (void)QueueStatus(OTA_STATUS_BAD_OFFSET, 0, 0u); return; }
    offset = ReadU32Le(payload);
    data_length = (uint16_t)(length - 4u);
    if ((data_length > OTA_ACCEPTED_CHUNK_SIZE) ||
        (offset > s_ota.artifact_size) ||
        ((uint32_t)data_length > (s_ota.artifact_size - offset))) {
        (void)QueueStatus(OTA_STATUS_BAD_OFFSET, 0, 0u); return;
    }
    if ((offset == s_ota.last_offset) && (data_length == s_ota.last_length) &&
        (s_ota.next_offset != 0u) &&
        (memcmp((const void *)(OTA_STAGE_BASE + offset), &payload[4], data_length) == 0)) {
        WriteU32Le(next, s_ota.next_offset); (void)QueueStatus(OTA_STATUS_ACK, next, 4u); return;
    }
    if ((offset != s_ota.next_offset) || ((offset & 1u) != 0u) ||
        (((data_length & 1u) != 0u) && (offset + data_length != s_ota.artifact_size))) {
        (void)QueueStatus(OTA_STATUS_BAD_OFFSET, 0, 0u); return;
    }
    if (ProgramBytes(OTA_STAGE_BASE + offset, &payload[4], data_length) == 0u) {
        (void)QueueStatus(OTA_STATUS_STORAGE, 0, 0u); ReturnIdle(); return;
    }
    s_ota.last_offset = offset;
    s_ota.last_length = data_length;
    s_ota.next_offset = offset + data_length;
    s_ota.timeout_ticks = 0u;
    s_ota.state = OTA_STATE_TRANSFERRING;
    WriteU32Le(next, s_ota.next_offset);
    (void)QueueStatus(OTA_STATUS_ACK, next, 4u);
}

static void HandleCommit(const uint8_t *payload, uint16_t length)
{
    uint32_t commit_crc;
    if ((s_ota.state != OTA_STATE_TRANSFERRING) || (length != 4u)) {
        (void)QueueStatus(OTA_STATUS_BUSY, 0, 0u); return;
    }
    commit_crc = ReadU32Le(payload);
    if ((s_ota.next_offset != s_ota.artifact_size) ||
        (commit_crc != s_ota.expected_crc) ||
        (FlashCrc32(OTA_STAGE_BASE, s_ota.artifact_size) != s_ota.expected_crc)) {
        QueueReason(OTA_STATUS_VERIFY_FAILED, OTA_REASON_CRC); ReturnIdle(); return;
    }
    if (ApplicationVectorValid(s_ota.artifact_size - OTA_SIGNATURE_SIZE) == 0u) {
        QueueReason(OTA_STATUS_VERIFY_FAILED, OTA_REASON_UNKNOWN); ReturnIdle(); return;
    }
    if (WriteMetadata() == 0u) {
        (void)QueueStatus(OTA_STATUS_STORAGE, 0, 0u); ReturnIdle(); return;
    }
    (void)QueueStatus(OTA_STATUS_VERIFY_OK, 0, 0u);
    (void)QueueStatus(OTA_STATUS_APPLYING, 0, 0u);
    s_ota.state = OTA_STATE_APPLYING;
    s_ota.reset_ticks = 0u;
}

void OtaUpdate_Init(void)
{
    memset(&s_ota, 0, sizeof(s_ota));
}

void OtaUpdate_HandleDisconnect(void)
{
    if (s_ota.state != OTA_STATE_APPLYING) { ReturnIdle(); }
}

uint8_t OtaUpdate_HandleFrame(uint8_t type, const uint8_t *payload, uint16_t length)
{
    if ((length != 0u) && (payload == 0)) { return 0u; }
    switch (type) {
        case OTA_TYPE_BEGIN: HandleBegin(payload, length); return 1u;
        case OTA_TYPE_CHUNK: HandleChunk(payload, length); return 1u;
        case OTA_TYPE_COMMIT: HandleCommit(payload, length); return 1u;
        case OTA_TYPE_ABORT:
            if (length == 0u) { AbortSession(0u); }
            else { QueueReason(OTA_STATUS_ABORTED, OTA_REASON_UNKNOWN); ReturnIdle(); }
            return 1u;
        default: return 0u;
    }
}

void OtaUpdate_Task20ms(void)
{
    if ((s_ota.state == OTA_STATE_READY) || (s_ota.state == OTA_STATE_TRANSFERRING)) {
        if (++s_ota.timeout_ticks >= OTA_TIMEOUT_TICKS_20MS) { AbortSession(OTA_REASON_TIMEOUT); }
    } else if (s_ota.state == OTA_STATE_APPLYING) {
        if (s_ota.reset_ticks < OTA_RESET_DELAY_TICKS) { s_ota.reset_ticks++; }
        if ((s_ota.reset_ticks >= OTA_RESET_DELAY_TICKS) && (Wireless_ProtocolTxIdle() != 0u)) {
            NVIC_SystemReset();
        }
    }
}

uint8_t OtaUpdate_IsActive(void)
{
    return (s_ota.state == OTA_STATE_IDLE) ? 0u : 1u;
}
