#ifndef BATTERY_TEST_HAL_H
#define BATTERY_TEST_HAL_H
#include <stdint.h>
typedef struct { volatile uint32_t SCR; } TestSCB;
extern TestSCB test_scb;
#define SCB (&test_scb)
#define SCB_SCR_SLEEPDEEP_Msk (4u)
#define SCB_SCR_SLEEPONEXIT_Msk (2u)
#define __get_PRIMASK test_get_primask
#define __get_IPSR test_get_ipsr
#define __disable_irq test_disable_irq
#define __set_PRIMASK test_set_primask
#define __DSB test_dsb
#define __ISB test_isb
#define __WFI test_wfi
uint32_t test_get_primask(void);
uint32_t test_get_ipsr(void);
void test_disable_irq(void);
void test_set_primask(uint32_t value);
void test_dsb(void);
void test_isb(void);
void test_wfi(void);
uint32_t HAL_GetTick(void);
#endif
