#ifndef MATHIS_OTA_UPDATE_H
#define MATHIS_OTA_UPDATE_H

#include <stdint.h>

void OtaUpdate_Init(void);
void OtaUpdate_HandleDisconnect(void);
uint8_t OtaUpdate_HandleFrame(uint8_t type, const uint8_t *payload, uint16_t length);
void OtaUpdate_Task20ms(void);
uint8_t OtaUpdate_IsActive(void);

#endif
