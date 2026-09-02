#ifndef MATHIS_BOOT_API_H
#define MATHIS_BOOT_API_H

#include <stdint.h>

#define MATHIS_BOOT_API_ADDRESS      (0x08001FC0u)
#define MATHIS_BOOT_API_MAGIC        (0x4950414Du) /* "MAPI" */
#define MATHIS_BOOT_API_ABI_VERSION  (1u)

#define MATHIS_BOOT_CAP_DEV_KEY      (0x00000001u)
#define MATHIS_BOOT_CAP_PROD_KEY     (0x00000002u)
#define MATHIS_BOOT_CAP_ANTIROLLBACK (0x00000004u)

#define MATHIS_VERIFY_KEY_DEV        (0x00000001u)
#define MATHIS_VERIFY_KEY_PROD       (0x00000002u)

typedef uint32_t (*MathisVerifyArtifactFn)(uint32_t artifact_address,
                                           uint32_t application_size,
                                           uint32_t artifact_size);
typedef uint32_t (*MathisReadSecurityFloorFn)(void);

typedef struct
{
    uint32_t magic;
    uint16_t abi_version;
    uint16_t struct_size;
    uint32_t capabilities;
    uint32_t flash_layout_version;
    MathisVerifyArtifactFn verify_artifact;
    MathisReadSecurityFloorFn read_security_floor;
    uint8_t dev_key_fingerprint[32];
    uint32_t reserved;
} MathisBootApi;

static __inline const MathisBootApi *MathisBootApi_Get(void)
{
    const MathisBootApi *api = (const MathisBootApi *)MATHIS_BOOT_API_ADDRESS;
    uint32_t verify = (uint32_t)api->verify_artifact;
    uint32_t floor = (uint32_t)api->read_security_floor;
    if ((api->magic != MATHIS_BOOT_API_MAGIC) ||
        (api->abi_version != MATHIS_BOOT_API_ABI_VERSION) ||
        (api->struct_size != sizeof(MathisBootApi)) ||
        ((api->capabilities & (MATHIS_BOOT_CAP_DEV_KEY | MATHIS_BOOT_CAP_PROD_KEY)) == 0u) ||
        ((verify & 1u) == 0u) || (verify < 0x08000001u) || (verify >= MATHIS_BOOT_API_ADDRESS) ||
        ((floor & 1u) == 0u) || (floor < 0x08000001u) || (floor >= MATHIS_BOOT_API_ADDRESS)) {
        return (const MathisBootApi *)0;
    }
    return api;
}

#endif
