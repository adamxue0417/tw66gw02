#ifndef MATHIS_SYSTEM_TYPES_H
#define MATHIS_SYSTEM_TYPES_H
#include <stdint.h>

/* Shared application data layout; no peripheral or module includes. */
typedef enum 
{
	idle,	
	test,
	start_up,
	run,
	set,
	shutdown,
	warning,
	ota
}_work_process;
typedef struct 
{
	uint8_t     units;     //0 F  1 C	     
	uint8_t     bluetooth;
  uint16_t    rtd_temp;
	uint16_t    proble0_temp;
	uint16_t    proble1_temp;
  uint16_t    icon;
  int16_t     pt1000_temp[9];  
  uint16_t    InTempSet;	  
}_system_data;   

typedef struct {
    uint8_t RtdErr;
    uint8_t HighTempErr;
} ErrMessage;

#endif
