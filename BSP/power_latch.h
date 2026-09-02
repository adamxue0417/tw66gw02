#ifndef MATHIS_POWER_LATCH_H
#define MATHIS_POWER_LATCH_H

#include "stm32f030x8.h"
#include <stdint.h>

/* The final 32 bytes of SRAM are excluded from every linker region.
 * 0x20001FE0..EF is retained across reset for trusted power intent and
 * 0x20001FF0..FF remains the existing development boot trace area. */
#define POWER_STATE_ADDRESS              (0x20001FE0u)
#define POWER_STATE_MAGIC                (0x31525750u) /* "PWR1" */
#define POWER_STATE_CHECK_XOR            (0xA53CC35Au)

#define POWER_STATE_RUNNING              (0x52554E31u) /* "RUN1" */
#define POWER_STATE_RESTART_ALLOWED      (0x52535431u) /* "RST1" */
#define POWER_STATE_SHUTDOWN_REQUESTED   (0x53444E31u) /* "SDN1" */

typedef struct {
    uint32_t magic;
    uint32_t state;
    uint32_t inverse;
    uint32_t check;
} PowerStateRecord;

static __inline uint32_t PowerLatch_CheckValue(uint32_t state)
{
    return POWER_STATE_MAGIC ^ (state * 0x9E3779B1u) ^ (~state) ^ POWER_STATE_CHECK_XOR;
}

static __inline uint8_t PowerLatch_KnownState(uint32_t state)
{
    return ((state == POWER_STATE_RUNNING) ||
            (state == POWER_STATE_RESTART_ALLOWED) ||
            (state == POWER_STATE_SHUTDOWN_REQUESTED)) ? 1u : 0u;
}

static __inline uint8_t PowerLatch_ReadState(uint32_t *state)
{
    const volatile PowerStateRecord *record =
        (const volatile PowerStateRecord *)POWER_STATE_ADDRESS;
    uint32_t value = record->state;
    if ((record->magic != POWER_STATE_MAGIC) ||
        (PowerLatch_KnownState(value) == 0u) ||
        (record->inverse != ~value) ||
        (record->check != PowerLatch_CheckValue(value))) {
        return 0u;
    }
    if (state != 0) { *state = value; }
    return 1u;
}

static __inline void PowerLatch_WriteState(uint32_t state)
{
    volatile PowerStateRecord *record = (volatile PowerStateRecord *)POWER_STATE_ADDRESS;
    record->magic = 0u;
    record->state = state;
    record->inverse = ~state;
    record->check = PowerLatch_CheckValue(state);
    __DMB();
    record->magic = POWER_STATE_MAGIC;
    __DSB();
}

static __inline void PowerLatch_ClearState(void)
{
    ((volatile PowerStateRecord *)POWER_STATE_ADDRESS)->magic = 0u;
    __DSB();
}

static __inline void PowerLatch_PreparePins(void)
{
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    (void)RCC->AHBENR;
    /* KEY0 PB5 is active low.  The pull-up makes an open key deterministic. */
    GPIOB->MODER &= ~(3u << (5u * 2u));
    GPIOB->PUPDR = (GPIOB->PUPDR & ~(3u << (5u * 2u))) |
                   (1u << (5u * 2u));
}

static __inline uint8_t PowerLatch_KeyPressed(void)
{
    return ((GPIOB->IDR & GPIO_IDR_5) == 0u) ? 1u : 0u;
}

static __inline void PowerLatch_DriveHigh(void)
{
    PowerLatch_PreparePins();
    GPIOB->BSRR = GPIO_BSRR_BS_3;
    GPIOB->MODER = (GPIOB->MODER & ~(3u << (3u * 2u))) |
                   (1u << (3u * 2u));
    GPIOB->OTYPER &= ~GPIO_OTYPER_OT_3;
}

static __inline void PowerLatch_DriveLow(void)
{
    PowerLatch_PreparePins();
    GPIOB->BSRR = GPIO_BSRR_BR_3;
    GPIOB->MODER = (GPIOB->MODER & ~(3u << (3u * 2u))) |
                   (1u << (3u * 2u));
    GPIOB->OTYPER &= ~GPIO_OTYPER_OT_3;
}

static __inline uint8_t PowerLatch_IsAlreadyHeld(void)
{
    uint32_t mode = (GPIOB->MODER >> (3u * 2u)) & 3u;
    return ((mode == 1u) && ((GPIOB->ODR & GPIO_ODR_3) != 0u)) ? 1u : 0u;
}

static __inline uint8_t PowerLatch_IsPowerReset(uint32_t reset_flags)
{
    return ((reset_flags & (RCC_CSR_PORRSTF | RCC_CSR_LPWRRSTF |
                            RCC_CSR_V18PWRRSTF)) != 0u) ? 1u : 0u;
}

static __inline void PowerLatch_SoftwareReset(void)
{
    PowerLatch_WriteState(POWER_STATE_RESTART_ALLOWED);
    __DSB();
    NVIC_SystemReset();
}

#endif /* MATHIS_POWER_LATCH_H */
