#ifndef MATHIS_OTA_UPDATE_INTERNAL_H
#define MATHIS_OTA_UPDATE_INTERNAL_H

/* Private session layout and timing; include only from ota_update.c. */
#include <stdint.h>

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

#endif
