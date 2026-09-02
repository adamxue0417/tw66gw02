#include "stm32f030x8.h"
#include "boot_security.h"
#include "ota_layout.h"
#include "power_latch.h"

#define BOOT_ERROR_MASK       (FLASH_SR_PGERR | FLASH_SR_WRPERR)
#define BOOT_LOG_TOKEN_BASE   (0x5A00u)

#define BOOT_TRACE(value)     (*(volatile uint32_t *)OTA_BOOT_TRACE_ADDRESS = (value))

static uint32_t Crc32Update(uint32_t crc, const uint8_t *data, uint32_t length);
static uint8_t HeaderValid(const OtaMetadataHeader *header);

static void PowerDeniedWait(void)
{
    PowerLatch_DriveLow();
    RCC->CSR |= RCC_CSR_RMVF;
    __disable_irq();
    while (1) { __WFI(); }
}

/* Called directly by Reset_Handler, before the C runtime initializes RAM. */
void SystemInit(void)
{
    const OtaMetadataHeader *header = (const OtaMetadataHeader *)OTA_METADATA_BASE;
    uint32_t reset_flags;
    uint32_t state = 0u;
    uint8_t state_valid;
    uint8_t recovery_reset;

    PowerLatch_PreparePins();
    reset_flags = RCC->CSR;
    state_valid = PowerLatch_ReadState(&state);

    /* Physical KEY0 always has highest priority and clears a stale shutdown. */
    if (PowerLatch_KeyPressed() != 0u) {
        PowerLatch_ClearState();
        PowerLatch_WriteState(POWER_STATE_RUNNING);
        PowerLatch_DriveHigh();
        RCC->CSR |= RCC_CSR_RMVF;
        return;
    }
    if ((state_valid != 0u) && (state == POWER_STATE_SHUTDOWN_REQUESTED)) {
        PowerDeniedWait();
    }
    if ((state_valid != 0u) && (state == POWER_STATE_RESTART_ALLOWED) &&
        ((reset_flags & RCC_CSR_SFTRSTF) != 0u)) {
        PowerLatch_WriteState(POWER_STATE_RUNNING); /* consume one-shot grant */
        PowerLatch_DriveHigh();
        RCC->CSR |= RCC_CSR_RMVF;
        return;
    }
    recovery_reset = ((reset_flags & (RCC_CSR_SFTRSTF | RCC_CSR_IWDGRSTF |
                                      RCC_CSR_WWDGRSTF | RCC_CSR_PINRSTF)) != 0u) ? 1u : 0u;
    if ((recovery_reset != 0u) && (HeaderValid(header) != 0u)) {
        PowerLatch_WriteState(POWER_STATE_RUNNING);
        PowerLatch_DriveHigh();
        RCC->CSR |= RCC_CSR_RMVF;
        return;
    }
    if ((state_valid != 0u) && (state == POWER_STATE_RUNNING) &&
        ((reset_flags & RCC_CSR_PINRSTF) != 0u) &&
        (PowerLatch_IsPowerReset(reset_flags) == 0u)) {
        PowerLatch_DriveHigh();
        RCC->CSR |= RCC_CSR_RMVF;
        return;
    }
    PowerDeniedWait();
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
    uint32_t i;
    uint8_t bit;
    for (i = 0u; i < length; i++) {
        crc ^= data[i];
        for (bit = 0u; bit < 8u; bit++) {
            crc = (crc & 1u) ? ((crc >> 1) ^ 0xEDB88320u) : (crc >> 1);
        }
    }
    return crc;
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
    uint32_t stack;
    uint32_t reset;
    if ((length < OTA_VECTOR_BYTES) || (length > OTA_MAX_APPLICATION_SIZE)) { return 0u; }
    stack = *(const uint32_t *)address;
    reset = *(const uint32_t *)(address + 4u);
    if ((stack < OTA_APP_RAM_BASE) || (stack > OTA_APP_RAM_END) || ((stack & 3u) != 0u)) { return 0u; }
    if ((reset & 1u) == 0u) { return 0u; }
    reset &= ~1u;
    return ((reset >= OTA_APP_BASE) && (reset < (OTA_APP_BASE + length))) ? 1u : 0u;
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

static uint8_t AddressInSwapStorage(uint32_t address)
{
    return ((address >= OTA_APP_BASE) && (address < OTA_SECURITY_BASE)) ? 1u : 0u;
}

static uint8_t AddressInBootMetadata(uint32_t address)
{
    return (((address >= OTA_SCRATCH_BASE) && (address < (OTA_METADATA_BASE + OTA_PAGE_SIZE)))) ? 1u : 0u;
}

static uint8_t FlashErasePage(uint32_t address)
{
    if (((AddressInSwapStorage(address) == 0u) && (AddressInBootMetadata(address) == 0u)) ||
        ((address & (OTA_PAGE_SIZE - 1u)) != 0u)) { return 0u; }
    if (FlashWait() == 0u) { return 0u; }
    FLASH->CR |= FLASH_CR_PER; FLASH->AR = address; FLASH->CR |= FLASH_CR_STRT;
    if (FlashWait() == 0u) { FLASH->CR &= ~FLASH_CR_PER; return 0u; }
    FLASH->CR &= ~FLASH_CR_PER;
    return 1u;
}

static uint8_t FlashProgramHalfwordRaw(uint32_t address, uint16_t value)
{
    uint16_t current;
    if ((address & 1u) != 0u) { return 0u; }
    current = *(const uint16_t *)address;
    if (current == value) { return 1u; }
    if (current != 0xFFFFu) { return 0u; }
    if (FlashWait() == 0u) { return 0u; }
    FLASH->CR |= FLASH_CR_PG;
    *(volatile uint16_t *)address = value;
    if (FlashWait() == 0u) { FLASH->CR &= ~FLASH_CR_PG; return 0u; }
    FLASH->CR &= ~FLASH_CR_PG;
    return (*(const uint16_t *)address == value) ? 1u : 0u;
}

static uint8_t FlashProgramHalfword(uint32_t address, uint16_t value)
{
    if ((AddressInSwapStorage(address) == 0u) && (AddressInBootMetadata(address) == 0u)) { return 0u; }
    return FlashProgramHalfwordRaw(address, value);
}

static uint16_t SecurityToken(uint8_t version)
{
    return (uint16_t)(0xA500u | version);
}

static uint8_t SecurityProgramVersion(uint8_t version)
{
    uint32_t index = (uint32_t)version - OTA_SECURITY_BASELINE;
    uint32_t a = OTA_SECURITY_BASE + OTA_SECURITY_COPY_A_OFFSET + index * 2u;
    uint32_t b = OTA_SECURITY_BASE + OTA_SECURITY_COPY_B_OFFSET + index * 2u;
    uint16_t token = SecurityToken(version);
    if ((index >= OTA_SECURITY_ENTRY_COUNT) || (FlashProgramHalfwordRaw(a, token) == 0u)) { return 0u; }
    return FlashProgramHalfwordRaw(b, token);
}

static uint8_t SecurityEnsureInitialized(void)
{
    uint32_t floor = BootSecurity_ReadFloor();
    if (floor != 0xFFFFFFFFu) { return 1u; }
    if (SecurityProgramVersion(OTA_SECURITY_BASELINE) == 0u) { return 0u; }
    if (FlashProgramHalfwordRaw(OTA_SECURITY_BASE + 8u, OTA_SECURITY_FORMAT) == 0u) { return 0u; }
    if (FlashProgramHalfwordRaw(OTA_SECURITY_BASE + 4u, (uint16_t)OTA_SECURITY_MAGIC_INV) == 0u) { return 0u; }
    if (FlashProgramHalfwordRaw(OTA_SECURITY_BASE + 6u, (uint16_t)(OTA_SECURITY_MAGIC_INV >> 16)) == 0u) { return 0u; }
    if (FlashProgramHalfwordRaw(OTA_SECURITY_BASE, (uint16_t)OTA_SECURITY_MAGIC) == 0u) { return 0u; }
    if (FlashProgramHalfwordRaw(OTA_SECURITY_BASE + 2u, (uint16_t)(OTA_SECURITY_MAGIC >> 16)) == 0u) { return 0u; }
    return (BootSecurity_ReadFloor() == OTA_SECURITY_BASELINE) ? 1u : 0u;
}

static uint8_t SecurityAdvance(uint8_t target)
{
    uint32_t floor = BootSecurity_ReadFloor();
    uint32_t version;
    if ((floor == 0xFFFFFFFFu) || (target < OTA_SECURITY_BASELINE)) { return 0u; }
    if (target <= floor) { return 1u; }
    for (version = floor + 1u; version <= target; version++) {
        if (SecurityProgramVersion((uint8_t)version) == 0u) { return 0u; }
    }
    return (BootSecurity_ReadFloor() >= target) ? 1u : 0u;
}

static uint8_t CopyPage(uint32_t destination, uint32_t source, uint32_t valid_bytes)
{
    uint32_t offset;
    uint16_t value;
    if (FlashErasePage(destination) == 0u) { return 0u; }
    for (offset = 0u; offset < OTA_PAGE_SIZE; offset += 2u) {
        value = (offset < valid_bytes) ? *(const uint16_t *)(source + offset) : 0xFFFFu;
        if ((value != 0xFFFFu) && (FlashProgramHalfword(destination + offset, value) == 0u)) { return 0u; }
    }
    for (offset = 0u; offset < OTA_PAGE_SIZE; offset += 2u) {
        value = (offset < valid_bytes) ? *(const uint16_t *)(source + offset) : 0xFFFFu;
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
    PowerLatch_WriteState(POWER_STATE_RUNNING);
    BOOT_TRACE(OTA_BOOT_TRACE_REMAP_OK);
    BranchToApplication(stack, reset);
    while (1) {}
}

int main(void)
{
    const OtaMetadataHeader *header = (const OtaMetadataHeader *)OTA_METADATA_BASE;
    uint8_t valid;
    if (PowerLatch_IsAlreadyHeld() == 0u) { PowerDeniedWait(); }
    BOOT_TRACE(OTA_BOOT_TRACE_ENTERED);
    valid = HeaderValid(header);
    if (FlashUnlock() == 0u) { JumpToApplication(); }
    if (SecurityEnsureInitialized() == 0u) {
        BOOT_TRACE(0xB00700E1u);
        while (1) {}
    }
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
                                (header->target_version >= BootSecurity_ReadFloor()) &&
                                (ImageVectorValid(OTA_STAGE_BASE, header->application_size) != 0u) &&
                                (BootSecurity_VerifyArtifact(OTA_STAGE_BASE, header->application_size,
                                                             header->artifact_size) != 0u)) ? 1u : 0u;
            }
            if ((staged_valid != 0u) &&
                (RunSwap(OTA_META_FORWARD_LOG_OFFSET, header->application_size, 1u) != 0u)) {
                if (WriteMarker(OTA_META_TRIAL_OFFSET, OTA_MARKER_TRIAL, OTA_MARKER_TRIAL_INV) != 0u) {
                    JumpToApplication();
                }
                PowerLatch_SoftwareReset();
            }
            completed = CompletedSteps(OTA_META_FORWARD_LOG_OFFSET);
            if (completed != 0u) { PowerLatch_SoftwareReset(); }
            (void)FlashErasePage(OTA_METADATA_BASE);
        } else if ((trial != 0u) && (confirmed == 0u) && (rollback_done == 0u)) {
            if (RunSwap(OTA_META_ROLLBACK_LOG_OFFSET, OTA_APP_SLOT_SIZE, 0u) != 0u) {
                if (WriteMarker(OTA_META_ROLLBACK_OFFSET, OTA_MARKER_ROLLBACK_DONE,
                                OTA_MARKER_ROLLBACK_DONE_INV) != 0u) {
                    (void)FlashErasePage(OTA_METADATA_BASE);
                    JumpToApplication();
                }
            }
            PowerLatch_SoftwareReset();
        } else if (rollback_done != 0u) {
            (void)FlashErasePage(OTA_METADATA_BASE);
            JumpToApplication();
        } else if ((trial != 0u) && (confirmed != 0u)) {
            if (SecurityAdvance(header->target_version) == 0u) {
                BOOT_TRACE(0xB00700E2u);
                while (1) {}
            }
            JumpToApplication();
        } else {
            JumpToApplication();
        }
    }
    JumpToApplication();
    return 0;
}
