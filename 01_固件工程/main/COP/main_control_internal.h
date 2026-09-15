#ifndef MATHIS_MAIN_CONTROL_INTERNAL_H
#define MATHIS_MAIN_CONTROL_INTERNAL_H

/* Private persistence ABI and idle timing; include only from main_control.c. */
#include <stdint.h>
#include "main_control.h"
#include "ota_layout.h"

#define IDLE_AUTO_SHUTDOWN_TICKS_100MS (36000u)
#define CONFIG_PAGE_A_ADDR              (OTA_CONFIG_A_BASE)
#define CONFIG_PAGE_B_ADDR              (OTA_CONFIG_B_BASE)
#define CONFIG_MAGIC                    (0x4D415448u)
#define CONFIG_VERSION                  (1u)
#define CONFIG_COMMIT                   (0xA55Au)

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t length;
    uint32_t sequence;
    uint8_t units;
    uint8_t reserved[3];
    float coeff[TEMP_COEFF_CHANNEL_COUNT][TEMP_COEFF_TERM_COUNT];
    uint16_t crc;
    uint16_t commit;
} PersistedConfig;

#endif
