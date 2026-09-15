#ifndef MATHIS_BOOT_INTERNAL_H
#define MATHIS_BOOT_INTERNAL_H

/* Bootloader-only flash/log/trace definitions, not an application API. */
#include "stm32f030x8.h"
#include "ota_layout.h"

#define BOOT_ERROR_MASK       (FLASH_SR_PGERR | FLASH_SR_WRPERR)
#define BOOT_LOG_TOKEN_BASE   (0x5A00u)

#define BOOT_TRACE(value)     (*(volatile uint32_t *)OTA_BOOT_TRACE_ADDRESS = (value))

#endif
