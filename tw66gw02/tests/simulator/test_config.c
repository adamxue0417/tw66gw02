#include "test_support.h"
#include "ota_layout.h"
static uint32_t config_pages[512];
#undef OTA_CONFIG_A_BASE
#undef OTA_CONFIG_B_BASE
#define OTA_CONFIG_A_BASE ((uint32_t)config_pages)
#define OTA_CONFIG_B_BASE ((uint32_t)config_pages + 1024u)
#include "../../COP/main_control.c"

void TestConfig(void)
{
    float coeff[5] = {0,0,0,1,0};
    uint32_t nan_bits = 0x7FC00000u;
    unsigned erases;
    memset(config_pages, 0xFF, sizeof(config_pages));
    system_data.units = unitC;
    CHECK(ConfigSave() == 1u);
    erases = mock_flash_erases;
    CHECK(ConfigSave() == 1u && mock_flash_erases == erases);
    CHECK(UI_SetUnits(unitF, 1u) == 1u);
    mock_flash_fail = 1; CHECK(UI_SetUnits(unitC, 1u) == 0u); CHECK(system_data.units == unitF);
    mock_flash_fail = 2; coeff[4] = 2.0f; CHECK(UI_SetTempCoeff(0u, coeff) == 0u);
    CHECK(s_temp_coeff[0][4] == 0.0f);
    mock_flash_fail = 0; CHECK(UI_SetTempCoeff(0u, coeff) == 1u);
    system_data.pt1000_temp[0] = 30;
    CHECK(UI_GetAdjustedMainTemp(0u) == 32);
    coeff[4] = 3.0f; CHECK(UI_SetTempCoeff(0u, coeff) == 1u);
    CHECK(UI_GetAdjustedMainTemp(0u) == 33);
    memcpy(&coeff[0], &nan_bits, sizeof(float)); CHECK(UI_SetTempCoeff(0u, coeff) == 0u);
    coeff[0] = 3.4e38f; CHECK(UI_SetTempCoeff(0u, coeff) == 1u);
    CHECK(UI_GetAdjustedMainTemp(0u) == HaveTempErr);
    mock_flash_fail = 3; CHECK(UI_SetUnits(unitC, 1u) == 0u);
    mock_flash_fail = 0;
    /* Sequence selection remains wrap-aware. */
    {
        PersistedConfig *active = (PersistedConfig *)s_config_active_page;
        active->sequence = 0xFFFFFFFFu;
        active->crc = ConfigCrc16((const uint8_t *)active, (uint16_t)offsetof(PersistedConfig, crc));
    }
    s_config_sequence = 0xFFFFFFFFu; system_data.units = unitC; CHECK(ConfigSave() == 1u);
    CHECK(s_config_sequence == 0u);
    s_config_active_page = 0u; ConfigLoad(); CHECK(system_data.units == unitC);
}
