#ifndef  __TASKSCHEDULER_H
#define  __TASKSCHEDULER_H
#include "stdint.h"

#define SCH_MAX_TASKS 32   //最大任务数量

//以下是错误代码
typedef enum
{
	NOT_ERROR = 0,
	ERROR_SCH_TOO_MANY_TASKS,
	ERROR_SCH_CANOT_DELETE_TASK,
	ERROR_SCH_WAITING_FOR_SLAVE_TO_ACK,
	ERROR_SCH_WAITING_FOR_START_COMMAND_FROM_MASTER,
	ERROR_SCH_ONE_OR_MORE_SLAVES_DID_NOT_START,
	ERROR_SCH_LOST_SLAVE,
	ERROR_SCH_CAN_BUS_ERROR,
	ERROR_I2C_WRITE_BYTE_AT34C64,
    ERROR_SCH_INVALID_TASK
}SCH_Error_TypeDef;

typedef struct //每个任务的数据结构
{
	void (* volatile pTask)(void);
	volatile uint16_t Delay;
	volatile uint16_t Preiod;
	volatile uint8_t RunMe;
}sTask;

void SCH_Init(void);
uint8_t   SCH_Delete_Task(uint16_t index);
void SCH_Dispatch_Tasks(void);
void SCH_Report_Status(void);
void SCH_Go_To_Sleep(void);
uint16_t SCH_Add_Task(void(*pFunction)(void), const uint16_t Delay, const uint16_t Period);
void SCH_Update(void) ;
void SCH_SysTick_Init(void);
#endif

