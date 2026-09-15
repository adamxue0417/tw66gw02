#ifndef MATHIS_WIRELESS_INTERNAL_H
#define MATHIS_WIRELESS_INTERNAL_H

/* Private queue/assembly types and timing; include only from wireless.c. */
#include <stdint.h>
#include "main_control.h"

#define TX_QUEUE_DEPTH              (4u)
#define TELEMETRY_PERIOD_TICKS_20MS (50u)
#define COEFF_TIMEOUT_TICKS_20MS    (3000u)
#define FW_RELEASE_NUMBER           ((uint8_t)system_version)

typedef struct {
    uint8_t data[20];
    uint8_t length;
} TxEntry;

typedef struct {
    float coeff[TEMP_COEFF_TERM_COUNT];
    uint8_t parts;
    uint16_t age;
} CoeffPending;

#endif
