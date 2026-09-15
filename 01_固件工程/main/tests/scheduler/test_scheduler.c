#include <stdio.h>
#include <stdlib.h>
#include "TaskScheduler.h"
#include "TaskScheduler_config.h"
#include "stm32f0xx_hal.h"

extern volatile sTask SCH_tasks_G[SCH_MAX_TASKS];
extern volatile SCH_Error_TypeDef Error_Code_G;
TestSCB test_scb;
static uint32_t mask, ipsr, tick, sleeps, calls, checks;
static uint32_t inject_before_lock, inject_at_dsb, pending, irq_delivered;
static uint32_t inject_in_task, callback_mask;

#define CHECK(x) do { checks++; if (!(x)) { \
    printf("FAIL line %u: %s\n", (unsigned)__LINE__, #x); exit(1); } } while (0)

void SystemInit(void) {}
uint32_t HAL_GetTick(void) { return tick; }
uint32_t test_get_primask(void) { return mask; }
uint32_t test_get_ipsr(void) { return ipsr; }
void test_disable_irq(void)
{
    if (inject_before_lock && !mask) {
        inject_before_lock = 0u;
        SCH_Update();
    }
    mask = 1u;
}
void test_set_primask(uint32_t value)
{
    mask = value;
    if (!mask && pending) {
        uint32_t kind = pending;
        pending = 0u;
        irq_delivered++;
        if (kind == 1u) { SCH_Update(); }
    }
}
void test_dsb(void)
{
    CHECK(mask == 1u);
    if (inject_at_dsb) { pending = inject_at_dsb; inject_at_dsb = 0u; }
}
void test_isb(void) { CHECK(mask == 0u); }
void test_wfi(void)
{
    /* Mock only the architectural boundary. No STM32 current/peripheral model. */
    CHECK(mask == 1u);
    CHECK((SCB->SCR & 6u) == 0u);
    sleeps++;
}
static void task(void)
{
    calls++;
    callback_mask = mask;
    if (inject_in_task) { inject_in_task = 0u; SCH_Update(); }
}
static void reset(void)
{
    mask = ipsr = tick = sleeps = calls = 0u;
    inject_before_lock = inject_at_dsb = pending = irq_delivered = 0u;
    inject_in_task = callback_mask = 0u;
    SCB->SCR = 0x16u;
    SCH_Init();
}

int main(void)
{
    uint32_t i;
    reset();
    SCH_Go_To_Sleep();
    CHECK(sleeps == SCH_IDLE_SLEEP_ENABLED);
    CHECK(mask == 0u);
#if SCH_IDLE_SLEEP_ENABLED
    CHECK(SCB->SCR == 0x10u);
#endif
    mask = 1u; SCH_Go_To_Sleep(); CHECK(mask == 1u);
    CHECK(sleeps == SCH_IDLE_SLEEP_ENABLED);
    mask = 0u; ipsr = 15u; SCH_Go_To_Sleep();
    CHECK(sleeps == SCH_IDLE_SLEEP_ENABLED);

    reset();
    CHECK(SCH_Add_Task(task, 0u, 10u) == 0u);
    SCH_Update();
    SCH_Go_To_Sleep(); CHECK(sleeps == 0u);
    SCH_Dispatch_Tasks(); CHECK(calls == 1u); CHECK(callback_mask == 0u);
    /* Lock the existing period+1 tick behavior; this change must not alter it. */
    for (i = 0; i < 10u; i++) { SCH_Update(); }
    CHECK(SCH_tasks_G[0].RunMe == 0u);
    SCH_Update(); CHECK(SCH_tasks_G[0].RunMe == 1u);

    reset(); SCH_Add_Task(task, 0u, 1u);
    for (i = 0; i < 5u; i++) { SCH_Update(); }
    CHECK(SCH_tasks_G[0].RunMe == 3u);
    SCH_Dispatch_Tasks(); CHECK(calls == 1u); CHECK(sleeps == 0u);
    SCH_Dispatch_Tasks(); CHECK(calls == 2u); CHECK(sleeps == 0u);
    SCH_Dispatch_Tasks(); CHECK(calls == 3u);
    CHECK(sleeps == SCH_IDLE_SLEEP_ENABLED);

    reset(); SCH_Add_Task(task, 0u, 1u); SCH_Update(); SCH_Update();
    inject_in_task = 1u;
    SCH_Dispatch_Tasks(); CHECK(calls == 1u);
    CHECK(SCH_tasks_G[0].RunMe == 1u); CHECK(sleeps == 0u);
    SCH_Dispatch_Tasks(); CHECK(calls == 2u);

    reset(); SCH_Add_Task(task, 0u, 0u);
    SCH_Update(); SCH_Update(); inject_in_task = 1u;
    SCH_Dispatch_Tasks(); SCH_Dispatch_Tasks();
    CHECK(calls == 1u); CHECK(SCH_tasks_G[0].pTask == 0);

#if SCH_IDLE_SLEEP_ENABLED
    reset(); SCH_Add_Task(task, 0u, 1u); inject_before_lock = 1u;
    SCH_Go_To_Sleep(); CHECK(sleeps == 0u); CHECK(SCH_tasks_G[0].RunMe == 1u);
    reset(); SCH_Add_Task(task, 0u, 1u); inject_at_dsb = 1u;
    SCH_Go_To_Sleep(); CHECK(sleeps == 1u); CHECK(irq_delivered == 1u);
    CHECK(SCH_tasks_G[0].RunMe == 1u); CHECK(mask == 0u);
    SCH_Dispatch_Tasks(); CHECK(calls == 1u);
    /* UART and DMA wake sources must be delivered after PRIMASK restoration. */
    for (i = 2u; i <= 3u; i++) {
        reset(); inject_at_dsb = i; SCH_Go_To_Sleep();
        CHECK(irq_delivered == 1u); CHECK(mask == 0u);
    }
    reset(); pending = 2u; SCH_Go_To_Sleep();
    CHECK(irq_delivered == 1u); CHECK(mask == 0u);
#endif

    reset(); SCH_Add_Task(task, 0u, 1u);
    for (i = 0; i < 520u; i++) { SCH_Update(); }
    CHECK(SCH_tasks_G[0].RunMe == 255u);
    CHECK(Error_Code_G == ERROR_SCH_PENDING_OVERFLOW);
    SCH_Go_To_Sleep(); CHECK(sleeps == 0u);

    reset(); tick = 0xFFFFFF00u;
    SCH_Delete_Task(SCH_MAX_TASKS); SCH_Report_Status();
    for (i = 0; i < 1000u; i++) { SCH_Report_Status(); }
    CHECK(Error_Code_G == ERROR_SCH_CANOT_DELETE_TASK);
    tick += 59999u; SCH_Report_Status(); CHECK(Error_Code_G != NOT_ERROR);
    tick += 1u; SCH_Report_Status(); CHECK(Error_Code_G == NOT_ERROR);
    SCH_Delete_Task(SCH_MAX_TASKS); SCH_Report_Status();
    tick += 30000u;
    SCH_Add_Task(0, 0u, 0u); SCH_Report_Status();
    tick += 30000u; SCH_Report_Status(); CHECK(Error_Code_G == ERROR_SCH_TOO_MANY_TASKS);
    tick += 30000u; SCH_Report_Status(); CHECK(Error_Code_G == NOT_ERROR);

    reset(); mask = 1u;
    SCH_Add_Task(task, 0u, 1u); CHECK(mask == 1u);
    SCH_Delete_Task(0u); CHECK(mask == 1u);
    SCH_Report_Status(); CHECK(mask == 1u);
    SCH_Init(); CHECK(mask == 1u); mask = 0u;
    for (i = 0; i < SCH_MAX_TASKS; i++) { CHECK(SCH_Add_Task(task, 0u, 1u) == i); }
    CHECK(SCH_Add_Task(task, 0u, 1u) == SCH_MAX_TASKS);
    CHECK(SCH_Delete_Task(SCH_MAX_TASKS) == SCH_RETURN_ERROR);
    printf("PASS: %u checks, SCH_IDLE_SLEEP_ENABLED=%u (mocked IRQ/WFI)\n",
           (unsigned)checks, (unsigned)SCH_IDLE_SLEEP_ENABLED);
    return 0;
}
