/* 应用侧开发版 OTA：IDLE→READY→TRANSFERRING→APPLYING，暂存镜像后交给 Bootloader 安装。 */
#include "config.h"
#include "ota_layout.h"
#include "ota_boot.h"
#include "ota_update.h"
#include "ota_update_internal.h"

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
    uint32_t i;
    uint8_t bit;
    for (i = 0u; i < length; i++) {
        crc ^= data[i];
        for (bit = 0u; bit < 8u; bit++) {
            crc = (crc & 1u) ? ((crc >> 1) ^ 0xEDB88320u) : (crc >> 1);
        }
    }
    return crc;
}

static uint32_t FlashCrc32(uint32_t address, uint32_t length)
{
    return Crc32Update(0xFFFFFFFFu, (const uint8_t *)address, length) ^ 0xFFFFFFFFu;
}

static uint8_t QueueStatus(uint8_t status, const uint8_t *extra, uint8_t extra_length)
{
    uint8_t payload[5];
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
    if ((address & 1u) != 0u) { return 0u; }
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
    uint32_t stack;
    uint32_t reset;
    if ((application_size < OTA_VECTOR_BYTES) || (application_size > OTA_MAX_APPLICATION_SIZE)) { return 0u; }
    stack = *(const uint32_t *)OTA_STAGE_BASE;
    reset = *(const uint32_t *)(OTA_STAGE_BASE + 4u);
    if ((stack < OTA_APP_RAM_BASE) || (stack > OTA_APP_RAM_END) || ((stack & 3u) != 0u)) { return 0u; }
    reset &= ~1u;
    return ((reset >= OTA_APP_BASE) && (reset < (OTA_APP_BASE + application_size))) ? 1u : 0u;
}

/* 元数据最后写 commit 半字；只有完整提交后 Bootloader 才能识别待安装镜像。 */
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

/* 总长度含签名占位；试运行未确认或电量不足 30% 时拒绝开始，准入通过后才擦除暂存区。 */
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
    if (snapshot.battery_percent < 30u) { (void)QueueStatus(OTA_STATUS_POWER, 0, 0u); return; }
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
    /* 只接受上一块偏移、长度及 Flash 内容均相同的重传，重复 ACK 而不重复写入。 */
    if ((offset == s_ota.last_offset) && (data_length == s_ota.last_length) &&
        (s_ota.next_offset != 0u) &&
        (memcmp((const void *)(OTA_STAGE_BASE + offset), &payload[4], data_length) == 0)) {
        WriteU32Le(next, s_ota.next_offset); (void)QueueStatus(OTA_STATUS_ACK, next, 4u); return;
    }
    if ((offset != s_ota.next_offset) || ((offset & 1u) != 0u)) {
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

/* 开发版仅检查接收完整性、CRC 和应用向量；签名尾部为占位，本路径未执行 RSA 验签。 */
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

/* 超时按无线任务调用次数推进；提交后等待发送队列空闲及复位延迟，再复位进入安装流程。 */
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
