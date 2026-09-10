#include "TaskScheduler.h"
#include "stm32f030x8.h"
#include <string.h>

/* SysTick produces pending work; the main loop is the only dispatcher.
 * Keep the historic Delay/Preiod countdown: period P runs every P + 1 ticks.
 * Preiod is intentionally retained for source compatibility.
 */
sTask SCH_tasks_G[SCH_MAX_TASKS];
SCH_Error_TypeDef Error_Code_G = NOT_ERROR;
static volatile uint16_t s_task_limit;
static uint16_t s_error_count;
static SCH_Error_TypeDef s_last_error;

static uint32_t LockTasks(void)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    return mask;
}

static void UnlockTasks(uint32_t mask)
{
    __set_PRIMASK(mask);
}

static void ClearTask(uint16_t index)
{
    SCH_tasks_G[index].pTask = 0;
    SCH_tasks_G[index].Delay = 0u;
    SCH_tasks_G[index].Preiod = 0u;
    SCH_tasks_G[index].RunMe = 0u;
    while ((s_task_limit != 0u) && (SCH_tasks_G[s_task_limit - 1u].pTask == 0)) {
        s_task_limit--;
    }
}

void SCH_Init(void)
{
    uint32_t mask = LockTasks();
    memset(SCH_tasks_G, 0, sizeof(SCH_tasks_G));
    s_task_limit = 0u;
    Error_Code_G = NOT_ERROR;
    s_last_error = NOT_ERROR;
    s_error_count = 0u;
    UnlockTasks(mask);
}

void SCH_Update(void)
{
    uint16_t index;
    for (index = 0u; index < s_task_limit; index++) {
        sTask *task = &SCH_tasks_G[index];
        if (task->pTask == 0) { continue; }
        if (task->Delay != 0u) {
            task->Delay--;
        } else {
            if ((task->Preiod != 0u) || (task->RunMe == 0u)) {
                if (task->RunMe != UINT8_MAX) { task->RunMe++; }
            }
            if (task->Preiod != 0u) { task->Delay = task->Preiod; }
        }
    }
}

uint16_t SCH_Add_Task(void (*function)(void), uint16_t delay, uint16_t period)
{
    uint16_t index = 0u;
    uint32_t mask = LockTasks();
    if (function == 0) {
        Error_Code_G = ERROR_SCH_INVALID_TASK;
        UnlockTasks(mask);
        return SCH_MAX_TASKS;
    }
    while ((index < SCH_MAX_TASKS) && (SCH_tasks_G[index].pTask != 0)) { index++; }
    if (index == SCH_MAX_TASKS) {
        Error_Code_G = ERROR_SCH_TOO_MANY_TASKS;
    } else {
        SCH_tasks_G[index].Delay = delay;
        SCH_tasks_G[index].Preiod = period;
        SCH_tasks_G[index].RunMe = 0u;
        SCH_tasks_G[index].pTask = function;
        if (index >= s_task_limit) { s_task_limit = index + 1u; }
    }
    UnlockTasks(mask);
    return index;
}

uint8_t SCH_Delete_Task(uint16_t index)
{
    uint8_t result = 1u;
    uint32_t mask = LockTasks();
    if ((index >= SCH_MAX_TASKS) || (SCH_tasks_G[index].pTask == 0)) {
        Error_Code_G = ERROR_SCH_CANOT_DELETE_TASK;
    } else {
        ClearTask(index);
        result = 0u;
    }
    UnlockTasks(mask);
    return result;
}

void SCH_Dispatch_Tasks(void)
{
    uint16_t index;
    for (index = 0u; index < s_task_limit; index++) {
        void (*function)(void) = 0;
        uint32_t mask = LockTasks();
        sTask *task = &SCH_tasks_G[index];
        if ((task->pTask != 0) && (task->RunMe != 0u)) {
            function = task->pTask;
            task->RunMe--;
            /* Retire a one-shot before calling it, so callback add/delete is safe. */
            if (task->Preiod == 0u) { ClearTask(index); }
        }
        UnlockTasks(mask);
        if (function != 0) { function(); }
    }
    SCH_Report_Status();
    SCH_Go_To_Sleep();
}

void SCH_Report_Status(void)
{
    /* Preserve the legacy dispatch-count error retention policy. */
    if (Error_Code_G != s_last_error) {
        s_last_error = Error_Code_G;
        s_error_count = (Error_Code_G != NOT_ERROR) ? 60000u : 0u;
    } else if (s_error_count != 0u) {
        if (--s_error_count == 0u) { Error_Code_G = NOT_ERROR; }
    }
}

void SCH_Go_To_Sleep(void)
{
    /* No WFI: retain existing interrupt and display timing. */
}
