#ifndef MATHIS_OTA_LAYOUT_H
#define MATHIS_OTA_LAYOUT_H

#include <stdint.h>

/* STM32F030C8: 64 KiB flash, 1 KiB erase pages. */
#define OTA_FLASH_BASE               (0x08000000u)
#define OTA_FLASH_END                (0x08010000u)
#define OTA_PAGE_SIZE                (0x00000400u)

#define OTA_BOOT_BASE                (0x08000000u)
#define OTA_BOOT_SIZE                (0x00002000u) /* 8 KiB, WRP pages 0..7 */
#define OTA_APP_BASE                 (0x08002000u)
#define OTA_APP_SLOT_SIZE            (0x00006400u) /* 25 KiB */
#define OTA_STAGE_BASE               (0x08008400u)
#define OTA_STAGE_SIZE               (0x00006400u) /* 25 KiB */
#define OTA_SECURITY_BASE            (0x0800E800u)
#define OTA_SECURITY_SIZE            (0x00000400u)
#define OTA_RESERVED_BASE            (0x0800EC00u)
#define OTA_SCRATCH_BASE             (0x0800F000u)
#define OTA_METADATA_BASE            (0x0800F400u)
#define OTA_CONFIG_A_BASE            (0x0800F800u)
#define OTA_CONFIG_B_BASE            (0x0800FC00u)

#define OTA_SIGNATURE_SIZE           (384u)
#define OTA_MAX_ARTIFACT_SIZE        (OTA_STAGE_SIZE)
#define OTA_MAX_APPLICATION_SIZE     (OTA_APP_SLOT_SIZE - OTA_SIGNATURE_SIZE)
#define OTA_ACCEPTED_CHUNK_SIZE      (232u)
#define OTA_VECTOR_BYTES             (192u)
#define OTA_APP_RAM_BASE             (0x200000C0u)
#define OTA_APP_RAM_END              (0x20001FE0u)
#define OTA_POWER_STATE_ADDRESS      (0x20001FE0u) /* 16-byte retained power intent */
#define OTA_BOOT_TRACE_ADDRESS       (0x20001FF0u) /* 16-byte development trace */
#define OTA_BOOT_TRACE_ENTERED       (0xB0070001u)
#define OTA_BOOT_TRACE_VECTOR_OK     (0xB0070002u)
#define OTA_BOOT_TRACE_REMAP_OK      (0xB0070003u)
#define OTA_BOOT_TRACE_APP_MAIN      (0xA9900001u)
#define OTA_BOOT_TRACE_HAL_READY     (0xA9900002u)
#define OTA_BOOT_TRACE_CLOCK_READY   (0xA9900003u)
#define OTA_BOOT_TRACE_PERIPH_READY  (0xA9900004u)
#define OTA_BOOT_TRACE_DATA_READY    (0xA9900005u)
#define OTA_BOOT_TRACE_SCREEN_READY  (0xA9900006u)
#define OTA_BOOT_TRACE_LOOP_RUNNING  (0xA9900007u)
#define OTA_BOOT_TRACE_APP_ERROR     (0xA99000EEu)

#define OTA_SECURITY_MAGIC           (0x31564E53u) /* "SNV1" little-endian */
#define OTA_SECURITY_MAGIC_INV       (0xCEA9B1ACu)
#define OTA_SECURITY_FORMAT          (1u)
#define OTA_SECURITY_BASELINE        (102u)
#define OTA_SECURITY_MAX_VERSION     (255u)
#define OTA_SECURITY_ENTRY_COUNT     (OTA_SECURITY_MAX_VERSION - OTA_SECURITY_BASELINE + 1u)
#define OTA_SECURITY_COPY_A_OFFSET   (32u)
#define OTA_SECURITY_COPY_B_OFFSET   (OTA_SECURITY_COPY_A_OFFSET + OTA_SECURITY_ENTRY_COUNT * 2u)

#define OTA_METADATA_MAGIC           (0x4F544131u) /* "OTA1" */
#define OTA_METADATA_FORMAT          (1u)
#define OTA_METADATA_COMMIT          (0xA55Au)
#define OTA_MARKER_TRIAL             (0x71A1u)
#define OTA_MARKER_TRIAL_INV         (0x8E5Eu)
#define OTA_MARKER_CONFIRMED         (0xC04Fu)
#define OTA_MARKER_CONFIRMED_INV     (0x3FB0u)
#define OTA_MARKER_ROLLBACK_DONE     (0xB44Bu)
#define OTA_MARKER_ROLLBACK_DONE_INV (0x4BB4u)

#define OTA_META_TRIAL_OFFSET        (32u)
#define OTA_META_CONFIRMED_OFFSET    (36u)
#define OTA_META_ROLLBACK_OFFSET     (40u)
#define OTA_META_FORWARD_LOG_OFFSET  (64u)
#define OTA_META_ROLLBACK_LOG_OFFSET (512u)
#define OTA_SLOT_PAGE_COUNT          (OTA_APP_SLOT_SIZE / OTA_PAGE_SIZE)
#define OTA_SWAP_STEP_COUNT          (OTA_SLOT_PAGE_COUNT * 3u)

typedef struct
{
    uint32_t magic;
    uint16_t format;
    uint16_t header_size;
    uint32_t artifact_size;
    uint32_t application_size;
    uint32_t artifact_crc32;
    uint8_t target_version;
    uint8_t flags;
    uint16_t reserved0;
    uint32_t header_crc32;
    uint16_t commit;
    uint16_t reserved1;
} OtaMetadataHeader;

#endif
