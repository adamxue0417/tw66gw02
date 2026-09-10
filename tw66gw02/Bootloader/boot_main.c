#include "stm32f030x8.h"
#include "ota_layout.h"
#include "mathis_util.h"

#define BOOT_ERROR_MASK       (FLASH_SR_PGERR | FLASH_SR_WRPERR)
#define BOOT_LOG_TOKEN_BASE   (0x5A00u)

#define BOOT_TRACE(value)     (*(volatile uint32_t *)OTA_BOOT_TRACE_ADDRESS = (value))

/* PB3 drives the board power-hold latch.  A push button only supplies the
 * initial pulse, so the bootloader must take ownership before doing any flash
 * recovery or jumping to the application. */
static void HoldBoardPower(void)
{
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    (void)RCC->AHBENR;
    GPIOB->BSRR = GPIO_BSRR_BS_3;
    GPIOB->MODER = (GPIOB->MODER & ~(3u << (3u * 2u))) |
                   (1u << (3u * 2u));
    GPIOB->OTYPER &= ~GPIO_OTYPER_OT_3;
}

/* Called directly by Reset_Handler, before the C runtime initializes RAM. */
void SystemInit(void)
{
    HoldBoardPower();
}

/* Change MSP and branch without allowing the C compiler to emit a stack
 * epilogue after MSP has changed.  CONTROL and PRIMASK are restored to their
 * reset-state values for the application. */
__asm static void BranchToApplication(uint32_t stack, uint32_t reset)
{
    MSR     MSP, r0
    MOVS    r0, #0
    MSR     CONTROL, r0
    ISB
    CPSIE   i
    BX      r1
}

static uint32_t Crc32Update(uint32_t crc, const uint8_t *data, uint32_t length)
{
    return Mathis_Crc32Update(crc, data, length);
}

static uint8_t MarkerValid(uint32_t offset, uint16_t marker, uint16_t inverse)
{
    const uint16_t *p = (const uint16_t *)(OTA_METADATA_BASE + offset);
    return ((p[0] == marker) && (p[1] == inverse)) ? 1u : 0u;
}

static uint8_t HeaderValid(const OtaMetadataHeader *header)
{
    uint32_t crc;
    if ((header->magic != OTA_METADATA_MAGIC) ||
        (header->format != OTA_METADATA_FORMAT) ||
        (header->header_size != sizeof(OtaMetadataHeader)) ||
        (header->commit != OTA_METADATA_COMMIT) ||
        (header->artifact_size <= OTA_SIGNATURE_SIZE) ||
        (header->artifact_size > OTA_MAX_ARTIFACT_SIZE) ||
        (header->application_size != (header->artifact_size - OTA_SIGNATURE_SIZE)) ||
        (header->application_size > OTA_MAX_APPLICATION_SIZE)) { return 0u; }
    crc = Crc32Update(0xFFFFFFFFu, (const uint8_t *)header, 24u) ^ 0xFFFFFFFFu;
    return (crc == header->header_crc32) ? 1u : 0u;
}

static uint8_t ImageVectorValid(uint32_t address, uint32_t length)
{
    return Mathis_AppVectorValid(*(const uint32_t *)address,
                                *(const uint32_t *)(address + 4u), length);
}

static uint8_t FlashWait(void)
{
    uint32_t timeout = 0x01000000u;
    while (((FLASH->SR & FLASH_SR_BSY) != 0u) && (--timeout != 0u)) {}
    if (timeout == 0u) { return 0u; }
    if ((FLASH->SR & BOOT_ERROR_MASK) != 0u) {
        FLASH->SR = BOOT_ERROR_MASK; return 0u;
    }
    if ((FLASH->SR & FLASH_SR_EOP) != 0u) { FLASH->SR = FLASH_SR_EOP; }
    return 1u;
}

static uint8_t FlashUnlock(void)
{
    if ((FLASH->CR & FLASH_CR_LOCK) != 0u) {
        FLASH->KEYR = FLASH_KEY1; FLASH->KEYR = FLASH_KEY2;
    }
    return ((FLASH->CR & FLASH_CR_LOCK) == 0u) ? 1u : 0u;
}

static uint8_t FlashErasePage(uint32_t address)
{
    if ((address < OTA_APP_BASE) || (address >= OTA_CONFIG_A_BASE) ||
        ((address & (OTA_PAGE_SIZE - 1u)) != 0u)) { return 0u; }
    if (FlashWait() == 0u) { return 0u; }
    FLASH->CR |= FLASH_CR_PER; FLASH->AR = address; FLASH->CR |= FLASH_CR_STRT;
    if (FlashWait() == 0u) { FLASH->CR &= ~FLASH_CR_PER; return 0u; }
    FLASH->CR &= ~FLASH_CR_PER;
    return 1u;
}

static uint8_t FlashProgramHalfword(uint32_t address, uint16_t value)
{
    if ((address < OTA_APP_BASE) || (address >= OTA_CONFIG_A_BASE) || ((address & 1u) != 0u)) { return 0u; }
    if (*(const uint16_t *)address == value) { return 1u; }
    if (*(const uint16_t *)address != 0xFFFFu) { return 0u; }
    if (FlashWait() == 0u) { return 0u; }
    FLASH->CR |= FLASH_CR_PG;
    *(volatile uint16_t *)address = value;
    if (FlashWait() == 0u) { FLASH->CR &= ~FLASH_CR_PG; return 0u; }
    FLASH->CR &= ~FLASH_CR_PG;
    return (*(const uint16_t *)address == value) ? 1u : 0u;
}

static uint8_t CopyPage(uint32_t destination, uint32_t source, uint32_t valid_bytes)
{
    uint32_t offset;
    uint16_t value;
    if ((valid_bytes > OTA_PAGE_SIZE) || ((source & (OTA_PAGE_SIZE - 1u)) != 0u) ||
        (Mathis_RangeWithin(source, OTA_PAGE_SIZE, OTA_APP_BASE, OTA_METADATA_BASE) == 0u)) { return 0u; }
    if (FlashErasePage(destination) == 0u) { return 0u; }
    for (offset = 0u; offset < OTA_PAGE_SIZE; offset += 2u) {
        value = Mathis_PageHalfword((const uint8_t *)source, offset, valid_bytes);
        if ((value != 0xFFFFu) && (FlashProgramHalfword(destination + offset, value) == 0u)) { return 0u; }
    }
    for (offset = 0u; offset < OTA_PAGE_SIZE; offset += 2u) {
        value = Mathis_PageHalfword((const uint8_t *)source, offset, valid_bytes);
        if (*(const uint16_t *)(destination + offset) != value) { return 0u; }
    }
    return 1u;
}

static uint16_t LogToken(uint16_t step)
{
    return (uint16_t)(BOOT_LOG_TOKEN_BASE | (step & 0x00FFu));
}

static uint16_t CompletedSteps(uint32_t log_offset)
{
    uint16_t step;
    const uint16_t *entry = (const uint16_t *)(OTA_METADATA_BASE + log_offset);
    for (step = 0u; step < OTA_SWAP_STEP_COUNT; step++) {
        uint16_t token = LogToken((uint16_t)(step + 1u));
        if ((entry[step * 2u] != token) || (entry[step * 2u + 1u] != (uint16_t)~token)) { break; }
    }
    return step;
}

static uint8_t AppendStep(uint32_t log_offset, uint16_t step)
{
    uint32_t address = OTA_METADATA_BASE + log_offset + ((uint32_t)(step - 1u) * 4u);
    uint16_t token = LogToken(step);
    if (FlashProgramHalfword(address, token) == 0u) { return 0u; }
    return FlashProgramHalfword(address + 2u, (uint16_t)~token);
}

static uint8_t WriteMarker(uint32_t offset, uint16_t marker, uint16_t inverse)
{
    if (FlashProgramHalfword(OTA_METADATA_BASE + offset, marker) == 0u) { return 0u; }
    return FlashProgramHalfword(OTA_METADATA_BASE + offset + 2u, inverse);
}

static uint8_t RunSwap(uint32_t log_offset, uint32_t incoming_size, uint8_t trim_incoming)
{
    uint16_t completed = CompletedSteps(log_offset);
    while (completed < OTA_SWAP_STEP_COUNT) {
        uint16_t page = (uint16_t)(completed / 3u);
        uint16_t phase = (uint16_t)(completed % 3u);
        uint32_t page_offset = (uint32_t)page * OTA_PAGE_SIZE;
        uint32_t valid = OTA_PAGE_SIZE;
        if ((trim_incoming != 0u) && (page_offset >= incoming_size)) { valid = 0u; }
        else if ((trim_incoming != 0u) && ((page_offset + OTA_PAGE_SIZE) > incoming_size)) {
            valid = incoming_size - page_offset;
        }
        if (phase == 0u) {
            if (CopyPage(OTA_SCRATCH_BASE, OTA_APP_BASE + page_offset, OTA_PAGE_SIZE) == 0u) { return 0u; }
        } else if (phase == 1u) {
            if (CopyPage(OTA_APP_BASE + page_offset, OTA_STAGE_BASE + page_offset, valid) == 0u) { return 0u; }
        } else {
            if (CopyPage(OTA_STAGE_BASE + page_offset, OTA_SCRATCH_BASE, OTA_PAGE_SIZE) == 0u) { return 0u; }
        }
        completed++;
        if (AppendStep(log_offset, completed) == 0u) { return 0u; }
    }
    return 1u;
}

static void JumpToApplication(void)
{
    uint32_t i;
    uint32_t stack;
    uint32_t reset;
    uint32_t *vectors = (uint32_t *)0x20000000u;
    /* The slot includes the 384-byte artifact signature reserve, while the
     * installed application does not.  Passing OTA_APP_SLOT_SIZE here makes
     * ImageVectorValid reject every normal application as oversized. */
    if (ImageVectorValid(OTA_APP_BASE, OTA_MAX_APPLICATION_SIZE) == 0u) {
        BOOT_TRACE(0xB00700EEu);
        while (1) {}
    }
    BOOT_TRACE(OTA_BOOT_TRACE_VECTOR_OK);
    for (i = 0u; i < (OTA_VECTOR_BYTES / 4u); i++) {
        vectors[i] = *(const uint32_t *)(OTA_APP_BASE + i * 4u);
    }
    stack = vectors[0]; reset = vectors[1];
    __disable_irq();
    SysTick->CTRL = 0u; SysTick->LOAD = 0u; SysTick->VAL = 0u;
    NVIC->ICER[0] = 0xFFFFFFFFu;
    NVIC->ICPR[0] = 0xFFFFFFFFu;
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk | SCB_ICSR_PENDSVCLR_Msk;
    FLASH->CR |= FLASH_CR_LOCK;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    (void)RCC->APB2ENR;
    SYSCFG->CFGR1 = (SYSCFG->CFGR1 & ~SYSCFG_CFGR1_MEM_MODE) | SYSCFG_CFGR1_MEM_MODE_0 | SYSCFG_CFGR1_MEM_MODE_1;
    __DSB(); __ISB();
    if ((SYSCFG->CFGR1 & SYSCFG_CFGR1_MEM_MODE) != SYSCFG_CFGR1_MEM_MODE) {
        BOOT_TRACE(0xB00700EFu);
        while (1) {}
    }
    BOOT_TRACE(OTA_BOOT_TRACE_REMAP_OK);
    BranchToApplication(stack, reset);
    while (1) {}
}

int main(void)
{
    const OtaMetadataHeader *header = (const OtaMetadataHeader *)OTA_METADATA_BASE;
    uint8_t valid;
    HoldBoardPower();
    BOOT_TRACE(OTA_BOOT_TRACE_ENTERED);
    valid = HeaderValid(header);
    if (FlashUnlock() == 0u) { JumpToApplication(); }
    if (valid != 0u) {
        uint8_t trial = MarkerValid(OTA_META_TRIAL_OFFSET, OTA_MARKER_TRIAL, OTA_MARKER_TRIAL_INV);
        uint8_t confirmed = MarkerValid(OTA_META_CONFIRMED_OFFSET, OTA_MARKER_CONFIRMED, OTA_MARKER_CONFIRMED_INV);
        uint8_t rollback_done = MarkerValid(OTA_META_ROLLBACK_OFFSET, OTA_MARKER_ROLLBACK_DONE, OTA_MARKER_ROLLBACK_DONE_INV);
        if ((trial == 0u) && (rollback_done == 0u)) {
            uint16_t completed = CompletedSteps(OTA_META_FORWARD_LOG_OFFSET);
            uint8_t staged_valid = 1u;
            if (completed == 0u) {
                uint32_t crc = Crc32Update(0xFFFFFFFFu, (const uint8_t *)OTA_STAGE_BASE,
                                           header->artifact_size) ^ 0xFFFFFFFFu;
                staged_valid = ((crc == header->artifact_crc32) &&
                                (ImageVectorValid(OTA_STAGE_BASE, header->application_size) != 0u)) ? 1u : 0u;
            }
            if ((staged_valid != 0u) &&
                (RunSwap(OTA_META_FORWARD_LOG_OFFSET, header->application_size, 1u) != 0u)) {
                if (WriteMarker(OTA_META_TRIAL_OFFSET, OTA_MARKER_TRIAL, OTA_MARKER_TRIAL_INV) != 0u) {
                    JumpToApplication();
                }
                NVIC_SystemReset();
            }
            completed = CompletedSteps(OTA_META_FORWARD_LOG_OFFSET);
            if (completed != 0u) { NVIC_SystemReset(); }
            (void)FlashErasePage(OTA_METADATA_BASE);
        } else if ((trial != 0u) && (confirmed == 0u) && (rollback_done == 0u)) {
            if (RunSwap(OTA_META_ROLLBACK_LOG_OFFSET, OTA_APP_SLOT_SIZE, 0u) != 0u) {
                if (WriteMarker(OTA_META_ROLLBACK_OFFSET, OTA_MARKER_ROLLBACK_DONE,
                                OTA_MARKER_ROLLBACK_DONE_INV) != 0u) {
                    (void)FlashErasePage(OTA_METADATA_BASE);
                    JumpToApplication();
                }
            }
            NVIC_SystemReset();
        } else if (rollback_done != 0u) {
            (void)FlashErasePage(OTA_METADATA_BASE);
            JumpToApplication();
        } else {
            JumpToApplication();
        }
    }
    JumpToApplication();
    return 0;
}
