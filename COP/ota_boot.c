#include "config.h"
#include "ota_layout.h"
#include "ota_boot.h"

static uint8_t MarkerValid(uint32_t address, uint16_t marker, uint16_t inverse)
{
    const volatile uint16_t *p = (const volatile uint16_t *)address;
    return ((p[0] == marker) && (p[1] == inverse)) ? 1u : 0u;
}

static uint8_t HeaderPresent(void)
{
    const OtaMetadataHeader *header = (const OtaMetadataHeader *)OTA_METADATA_BASE;
    return ((header->magic == OTA_METADATA_MAGIC) &&
            (header->format == OTA_METADATA_FORMAT) &&
            (header->header_size == sizeof(OtaMetadataHeader)) &&
            (header->commit == OTA_METADATA_COMMIT)) ? 1u : 0u;
}

uint8_t OtaBoot_IsTrial(void)
{
    if (HeaderPresent() == 0u) { return 0u; }
    if (MarkerValid(OTA_METADATA_BASE + OTA_META_TRIAL_OFFSET,
                    OTA_MARKER_TRIAL, OTA_MARKER_TRIAL_INV) == 0u) { return 0u; }
    return (MarkerValid(OTA_METADATA_BASE + OTA_META_CONFIRMED_OFFSET,
                        OTA_MARKER_CONFIRMED, OTA_MARKER_CONFIRMED_INV) == 0u) ? 1u : 0u;
}

uint8_t OtaBoot_CanStartUpdate(void)
{
    if (HeaderPresent() == 0u) { return 1u; }
    return MarkerValid(OTA_METADATA_BASE + OTA_META_CONFIRMED_OFFSET,
                       OTA_MARKER_CONFIRMED, OTA_MARKER_CONFIRMED_INV);
}

void OtaBoot_ConfirmRunningImage(void)
{
    uint32_t address;
    const volatile uint16_t *marker;
    if (OtaBoot_IsTrial() == 0u) { return; }
    address = OTA_METADATA_BASE + OTA_META_CONFIRMED_OFFSET;
    marker = (const volatile uint16_t *)address;
    if (HAL_FLASH_Unlock() != HAL_OK) { return; }
    if (((marker[0] == OTA_MARKER_CONFIRMED) ||
         (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, address, OTA_MARKER_CONFIRMED) == HAL_OK)) &&
        (marker[1] != OTA_MARKER_CONFIRMED_INV)) {
        (void)HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, address + 2u, OTA_MARKER_CONFIRMED_INV);
    }
    (void)HAL_FLASH_Lock();
}
