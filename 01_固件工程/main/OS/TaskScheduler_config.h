#ifndef MATHIS_TASK_SCHEDULER_CONFIG_H
#define MATHIS_TASK_SCHEDULER_CONFIG_H

/* 0: busy-idle control image; 1: shallow Sleep between ready tasks. */
#ifndef SCH_IDLE_SLEEP_ENABLED
#define SCH_IDLE_SLEEP_ENABLED (1u)
#endif

#if (SCH_IDLE_SLEEP_ENABLED != 0) && (SCH_IDLE_SLEEP_ENABLED != 1)
#error SCH_IDLE_SLEEP_ENABLED_must_be_0_or_1
#endif

#define SCH_ERROR_HOLD_MS (60000u)

#endif
