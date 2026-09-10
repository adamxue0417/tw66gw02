#include "test_support.h"
#include "../../OS/TaskScheduler.c"
static unsigned calls;
static void Count(void) { calls++; }
static void TickInsideTask(void) { calls++; SCH_Update(); }

void TestScheduler(void)
{
    unsigned i;
    SCH_Init(); calls = 0u;
    CHECK(SCH_Add_Task(0, 0u, 1u) == SCH_MAX_TASKS);
    CHECK(SCH_Delete_Task(SCH_MAX_TASKS) == 1u);
    for (i = 0u; i < SCH_MAX_TASKS; i++) { CHECK(SCH_Add_Task(Count, 0u, 10u) == i); }
    CHECK(SCH_Add_Task(Count, 0u, 1u) == SCH_MAX_TASKS);
    CHECK(SCH_Delete_Task(31u) == 0u);
    CHECK(s_task_limit == 31u);
    SCH_Init(); CHECK(s_task_limit == 0u);
    SCH_Add_Task(Count, 0u, 10u);
    for (i = 0u; i < 22u; i++) { SCH_Update(); SCH_Dispatch_Tasks(); }
    CHECK(calls == 2u);
    SCH_Init(); calls = 0u;
    SCH_Add_Task(TickInsideTask, 0u, 0u);
    for (i = 0u; i < 1000u; i++) { SCH_Update(); }
    CHECK(SCH_tasks_G[0].RunMe == 1u);
    SCH_Dispatch_Tasks(); SCH_Dispatch_Tasks(); CHECK(calls == 1u);
    SCH_Init(); SCH_Add_Task(Count, 0u, 1u);
    for (i = 0u; i < 1000u; i++) { SCH_Update(); }
    CHECK(SCH_tasks_G[0].RunMe == 255u);
    SCH_Dispatch_Tasks(); CHECK(SCH_tasks_G[0].RunMe == 254u);
    __disable_irq(); SCH_Delete_Task(0u); CHECK(__get_PRIMASK() == 1u); __enable_irq();
    SCH_Init(); calls = 0u; SCH_Add_Task(TickInsideTask, 0u, 1u);
    SCH_Update(); SCH_Dispatch_Tasks();
    SCH_Update(); SCH_Dispatch_Tasks(); CHECK(calls == 2u);
    SCH_Init();
}
