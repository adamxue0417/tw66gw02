#include "stm32f030x8.h"
#include "ota_layout.h"

#define TRACE(value) (*(volatile uint32_t *)OTA_BOOT_TRACE_ADDRESS = (value))

static void HoldBoardPower(void)
{
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    (void)RCC->AHBENR;
    GPIOB->BSRR = GPIO_BSRR_BS_3;
    GPIOB->MODER = (GPIOB->MODER & ~(3u << 6u)) | (1u << 6u);
    GPIOB->OTYPER &= ~GPIO_OTYPER_OT_3;
}

void SystemInit(void)
{
    HoldBoardPower();
}

__asm static void BranchToApplication(uint32_t stack, uint32_t reset)
{
    MSR     MSP, r0
    MOVS    r0, #0
    MSR     CONTROL, r0
    ISB
    CPSIE   i
    BX      r1
}

int main(void)
{
    uint32_t i;
    uint32_t stack = *(const uint32_t *)OTA_APP_BASE;
    uint32_t reset = *(const uint32_t *)(OTA_APP_BASE + 4u);
    uint32_t *vectors = (uint32_t *)0x20000000u;

    HoldBoardPower();
    TRACE(0xB0071001u);
    if ((stack < OTA_APP_RAM_BASE) || (stack > OTA_APP_RAM_END) ||
        ((stack & 3u) != 0u) || ((reset & 1u) == 0u) ||
        ((reset & ~1u) < OTA_APP_BASE) ||
        ((reset & ~1u) >= (OTA_APP_BASE + OTA_APP_SLOT_SIZE))) {
        TRACE(0xB00710EEu);
        while (1) {}
    }

    __disable_irq();
    for (i = 0u; i < (OTA_VECTOR_BYTES / 4u); i++) {
        vectors[i] = *(const uint32_t *)(OTA_APP_BASE + i * 4u);
    }
    SysTick->CTRL = 0u;
    NVIC->ICER[0] = 0xFFFFFFFFu;
    NVIC->ICPR[0] = 0xFFFFFFFFu;
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk | SCB_ICSR_PENDSVCLR_Msk;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    (void)RCC->APB2ENR;
    SYSCFG->CFGR1 = (SYSCFG->CFGR1 & ~SYSCFG_CFGR1_MEM_MODE) |
                    SYSCFG_CFGR1_MEM_MODE;
    __DSB();
    __ISB();
    if ((SYSCFG->CFGR1 & SYSCFG_CFGR1_MEM_MODE) != SYSCFG_CFGR1_MEM_MODE) {
        TRACE(0xB00710EFu);
        while (1) {}
    }
    TRACE(0xB0071003u);
    BranchToApplication(stack, reset);
    while (1) {}
}
