#include "stm32f030x8.h"

static void BoardPowerAndOutputsInit(void)
{
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN;
    (void)RCC->AHBENR;

    /* Set output levels before changing modes: PB3 holds board power, PB8 is
       the buzzer, and PC13 is the run LED/test point used by the application. */
    GPIOB->BSRR = GPIO_BSRR_BS_3 | GPIO_BSRR_BR_8;
    GPIOC->BSRR = GPIO_BSRR_BR_13;
    GPIOB->MODER = (GPIOB->MODER & ~((3u << 6u) | (3u << 16u))) |
                   (1u << 6u) | (1u << 16u);
    GPIOC->MODER = (GPIOC->MODER & ~(3u << 26u)) | (1u << 26u);
    GPIOB->OTYPER &= ~((1u << 3u) | (1u << 8u));
    GPIOC->OTYPER &= ~(1u << 13u);
}

void SystemInit(void)
{
    BoardPowerAndOutputsInit();
}

static void BusyDelay(void)
{
    volatile uint32_t count;
    for (count = 0u; count < 800000u; count++) { __NOP(); }
}

int main(void)
{
    BoardPowerAndOutputsInit();
    while (1) {
        GPIOC->ODR ^= (1u << 13u);
        GPIOB->ODR ^= (1u << 8u);
        BusyDelay();
    }
}
