#ifndef MATHIS_BOOT_SECURITY_H
#define MATHIS_BOOT_SECURITY_H

#include <stdint.h>

uint32_t BootSecurity_VerifyArtifact(uint32_t artifact_address,
                                     uint32_t application_size,
                                     uint32_t artifact_size);
uint32_t BootSecurity_ReadFloor(void);

#endif
