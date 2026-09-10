#include "test_support.h"
#include "../../BSP/temp.c"

void TestTemperature(void)
{
    uint16_t values[10];
    uint32_t sum = 0u;
    unsigned i;
    MX_ADC_Init();
    adc_value[0] = 1000u; adc_value[1] = 1500u;
    mock_adc_mode = 0;
    CHECK(AcquireAdcBatch() == 1u);
    CHECK(ch_env[0].vol == 1000u && ch_env[1].vol == 1500u);
    CHECK(AcquireAdcBatch() == 1u);
    CHECK(mock_adc_starts == 1u && mock_adc_calibrations == 1u);
    mock_adc_mode = 2; CHECK(AcquireAdcBatch() == 0u); CHECK(g_battery_sample_valid == 0u);
    CHECK(mock_adc_stops == 1u);
    mock_adc_mode = 0; CHECK(AcquireAdcBatch() == 1u); CHECK(mock_adc_starts == 2u);
    mock_adc_mode = 1; CHECK(AcquireAdcBatch() == 0u);
    mock_adc_mode = 3; CHECK(AcquireAdcBatch() == 0u);
    mock_adc_mode = 0; CHECK(AcquireAdcBatch() == 1u);
    CHECK(Temp_Get(255u) == TempErr);
    CHECK(Average_Temp(0, 10u) == 0u); CHECK(Average_Temp(values, 0u) == 0u);
    for (i = 0u; i < 10u; i++) { values[i] = 65535u; }
    CHECK(Average_Temp(values, 10u) == 65535u);
    Add_Temp(0, 0u, 1u);
    for (i = 1u; i <= 100u; i++) {
        unsigned count = (i < 10u) ? i : 10u;
        sum += i;
        if (i > 10u) { sum -= i - 10u; }
        CHECK(ProbeAverage((uint16_t)i) == sum / count);
    }
    CHECK(raw_temp_filter(0u, -4) == -4);
    for (i = 0u; i < 20u; i++) { CHECK(raw_temp_filter(0u, -4) == -4); }
    CHECK(raw_temp_filter(0u, TempHigh) == TempHigh);
    CHECK(raw_temp_filter(0u, 100) == 100);
    CHECK(debounce_external_temp(1u, TempHigh) == 500);
    CHECK(debounce_external_temp(1u, TempHigh) == 500);
    CHECK(debounce_external_temp(1u, TempHigh) == TempHigh);
    CHECK(debounce_external_temp(1u, 100) == TempHigh);
    CHECK(debounce_external_temp(1u, 100) == 100);
    CHECK(debounce_external_temp(1u, TempDisconnected) == TempDisconnected);
    CHECK(PT1000_CalculateTemperature(10000u) == 0.0f);
    CHECK(PT1000_CalculateTemperature(28098u) == 500.0f);
}
