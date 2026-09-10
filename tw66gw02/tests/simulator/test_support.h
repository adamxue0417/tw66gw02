#ifndef MATHIS_TEST_SUPPORT_H
#define MATHIS_TEST_SUPPORT_H
#include "config.h"
#include "mathis_util.h"
#include <stdio.h>
#include <stdlib.h>
extern unsigned test_checks;
#define CHECK(value) do { test_checks++; if (!(value)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #value); exit(1); } } while (0)
void TestScheduler(void);
void TestUtilities(void);
void TestTemperature(void);
void TestWireless(void);
void TestConfig(void);
void TestOta(void);
extern int mock_flash_fail;
extern unsigned mock_flash_erases;
extern unsigned mock_flash_writes;
extern int mock_adc_mode;
extern const uint8_t *mock_tx_data;
extern uint16_t mock_tx_length;
extern unsigned mock_adc_starts;
extern unsigned mock_adc_calibrations;
extern unsigned mock_adc_stops;
#endif
