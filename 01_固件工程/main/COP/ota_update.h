#ifndef MATHIS_OTA_UPDATE_H
#define MATHIS_OTA_UPDATE_H

#include <stdint.h>

/* Wire protocol values; shared by transport users. */
#define OTA_TYPE_BEGIN             (0x10u)
#define OTA_TYPE_CHUNK             (0x11u)
#define OTA_TYPE_COMMIT            (0x12u)
#define OTA_TYPE_ABORT             (0x13u)
#define OTA_TYPE_STATUS            (0x90u)

#define OTA_STATUS_READY           (0x01u)
#define OTA_STATUS_ACK             (0x03u)
#define OTA_STATUS_VERIFY_OK       (0x04u)
#define OTA_STATUS_VERIFY_FAILED   (0x05u)
#define OTA_STATUS_APPLYING        (0x06u)
#define OTA_STATUS_ABORTED         (0x07u)
#define OTA_STATUS_POWER           (0x08u)
#define OTA_STATUS_STORAGE         (0x09u)
#define OTA_STATUS_BUSY            (0x0Au)
#define OTA_STATUS_BAD_OFFSET      (0x0Bu)

#define OTA_REASON_CRC             (0x01u)
#define OTA_REASON_SIZE            (0x03u)
#define OTA_REASON_TIMEOUT         (0x04u)
#define OTA_REASON_UNKNOWN         (0xFFu)

void OtaUpdate_Init(void);
void OtaUpdate_HandleDisconnect(void);
uint8_t OtaUpdate_HandleFrame(uint8_t type, const uint8_t *payload, uint16_t length);
void OtaUpdate_Task20ms(void);
uint8_t OtaUpdate_IsActive(void);

#endif
