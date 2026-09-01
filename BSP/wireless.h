/** Mathis BLE application protocol over the pre-configured EMB1061 UART. */
#ifndef __WIRELESS_H__
#define __WIRELESS_H__

#include "config.h"

#define MATHIS_START_BYTE          (0x5Au)
#define MATHIS_MAX_PAYLOAD         (236u)
#define MATHIS_MAX_FRAME           (244u)
#define MATHIS_TYPE_CMD            (0x01u)
#define MATHIS_TYPE_TELEMETRY      (0x81u)

#define MATHIS_CMD_SET_UNITS       (0x01u)
#define MATHIS_CMD_POWER_OFF       (0x02u)
#define MATHIS_CMD_COEFF_AB        (0x03u)
#define MATHIS_CMD_FACTORY_RESET   (0x04u)
#define MATHIS_CMD_COEFF_CD        (0x05u)
#define MATHIS_CMD_COEFF_E         (0x06u)

/* Framework-level telemetry control carried inside a valid CMD payload. */
#define MATHIS_CMD_TELEMETRY_CTRL  (0xFFu)
#define MATHIS_TELEMETRY_STOP      (0x00u)
#define MATHIS_TELEMETRY_START     (0x01u)

typedef struct {
    volatile uint32_t uart_chunks;
    volatile uint32_t uart_bytes;
    volatile uint32_t frames_ok;
    volatile uint32_t crc_errors;
    volatile uint32_t length_errors;
    volatile uint32_t cmd_frames;
    volatile uint32_t start_frames;
    volatile uint32_t stop_frames;
    volatile uint32_t module_start_events;
    volatile uint32_t module_stop_events;
    volatile uint32_t telemetry_queued;
    volatile uint32_t telemetry_tx_started;
    volatile uint16_t last_uart_length;
    volatile uint16_t last_payload_length;
    volatile uint8_t last_type;
    volatile uint8_t last_sequence;
    volatile uint8_t last_command;
    volatile uint8_t telemetry_enabled;
} MathisBleDebug;

extern MathisBleDebug g_mathis_ble_debug;

void Wireless_Init(void);
void WirelessTask(void);
uint16_t Mathis_Crc16(const uint8_t *data, uint16_t length);

#endif
