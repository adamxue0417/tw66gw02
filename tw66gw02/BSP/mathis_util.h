#ifndef MATHIS_UTIL_H
#define MATHIS_UTIL_H

#include <stdint.h>

/* CRC, finite-value and image-boundary helpers. Implementation: mathis_util.c. */
uint16_t Mathis_Crc16Bytes(const uint8_t *data, uint16_t length);

uint32_t Mathis_Crc32Update(uint32_t crc, const uint8_t *data, uint32_t length);

uint8_t Mathis_FloatFinite(float value);

uint8_t Mathis_RangeWithin(uint32_t address, uint32_t length,
                                         uint32_t base, uint32_t end);

uint8_t Mathis_AppVectorValid(uint32_t stack, uint32_t reset, uint32_t length);

uint16_t Mathis_PageHalfword(const uint8_t *source, uint32_t offset, uint32_t valid);
#endif
