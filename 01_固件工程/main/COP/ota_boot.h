#ifndef MATHIS_OTA_BOOT_H
#define MATHIS_OTA_BOOT_H

#include <stdint.h>

uint8_t OtaBoot_IsTrial(void);
uint8_t OtaBoot_CanStartUpdate(void);
void OtaBoot_ConfirmRunningImage(void);

#endif
