#include "mathis_util.h"
#include "ota_layout.h"
#include <string.h>

/* Pure helpers shared by the application and the HAL-free bootloader. */
uint16_t Mathis_Crc16Bytes(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFFu;
    uint8_t bit;
    while (length-- != 0u) {
        crc ^= (uint16_t)*data++ << 8;
        for (bit = 0u; bit < 8u; bit++) {
            crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u) : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

uint32_t Mathis_Crc32Update(uint32_t crc, const uint8_t *data, uint32_t length)
{
    uint8_t bit;
    while (length-- != 0u) {
        crc ^= *data++;
        for (bit = 0u; bit < 8u; bit++) {
            crc = (crc & 1u) ? ((crc >> 1) ^ 0xEDB88320u) : (crc >> 1);
        }
    }
    return crc;
}

uint8_t Mathis_FloatFinite(float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return ((bits & 0x7F800000u) != 0x7F800000u) ? 1u : 0u;
}

/* Subtraction avoids overflow in address + length for untrusted lengths. */
uint8_t Mathis_RangeWithin(uint32_t address, uint32_t length,
                                         uint32_t base, uint32_t end)
{
    return ((address >= base) && (address <= end) && (length <= end - address)) ? 1u : 0u;
}

uint8_t Mathis_AppVectorValid(uint32_t stack, uint32_t reset, uint32_t length)
{
    if ((length < OTA_VECTOR_BYTES) || (length > OTA_MAX_APPLICATION_SIZE) ||
        (stack < OTA_APP_RAM_BASE) || (stack > OTA_APP_RAM_END) ||
        ((stack & 3u) != 0u) || ((reset & 1u) == 0u)) { return 0u; }
    reset &= ~1u;
    return ((reset >= OTA_APP_BASE) && (reset - OTA_APP_BASE < length)) ? 1u : 0u;
}

/* Odd application tails must not copy the first signature byte. */
uint16_t Mathis_PageHalfword(const uint8_t *source, uint32_t offset, uint32_t valid)
{
    uint16_t value = 0xFFFFu;
    if (offset < valid) {
        value = source[offset];
        value |= (offset + 1u < valid) ? (uint16_t)source[offset + 1u] << 8 : 0xFF00u;
    }
    return value;
}
