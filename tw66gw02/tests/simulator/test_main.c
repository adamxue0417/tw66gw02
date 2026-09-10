#include "test_support.h"
#include "ota_layout.h"
unsigned test_checks;
void SystemInit(void) {}

void TestUtilities(void)
{
    static const uint8_t digits[] = "123456789";
    uint8_t bytes[4] = {0x12u, 0x34u, 0x56u, 0x78u};
    uint32_t bits = 0x7FC00000u;
    float nan;
    CHECK(Mathis_Crc16Bytes(digits, 9u) == 0x29B1u);
    CHECK((Mathis_Crc32Update(0xFFFFFFFFu, digits, 9u) ^ 0xFFFFFFFFu) == 0xCBF43926u);
    CHECK(Mathis_Crc16Bytes(0, 0u) == 0xFFFFu);
    CHECK(Mathis_RangeWithin(0xFFFFFFFEu, 4u, 0u, 0xFFFFFFFFu) == 0u);
    CHECK(Mathis_AppVectorValid(OTA_APP_RAM_END, OTA_APP_BASE + 193u, 256u) == 1u);
    CHECK(Mathis_AppVectorValid(OTA_APP_RAM_END, OTA_APP_BASE + 192u, 256u) == 0u);
    CHECK(Mathis_AppVectorValid(OTA_APP_RAM_END + 4u, OTA_APP_BASE + 193u, 256u) == 0u);
    CHECK(Mathis_AppVectorValid(OTA_APP_RAM_END, OTA_APP_BASE + 257u, 256u) == 0u);
    CHECK(Mathis_PageHalfword(bytes, 2u, 3u) == 0xFF56u);
    CHECK(Mathis_PageHalfword(bytes, 0u, 1u) == 0xFF12u);
    CHECK(Mathis_PageHalfword(bytes, 2u, 2u) == 0xFFFFu);
    memcpy(&nan, &bits, sizeof(nan));
    CHECK(Mathis_FloatFinite(nan) == 0u);
    CHECK(Mathis_FloatFinite(-123.5f) == 1u);
}

int main(void)
{
    TestUtilities();
    TestScheduler();
    TestTemperature();
    TestConfig();
    TestWireless();
    TestOta();
    printf("PASS: %u checks against compiled C implementations\n", test_checks);
    return 0;
}
