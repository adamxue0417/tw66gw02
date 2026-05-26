/**
  ******************************************************************************
  * @file    wireless.h
  * @author 
  * @version V1.0
  * @date
  * @brief   Wireless protocol module interface definitions.
  ******************************************************************************
  */
#ifndef __WIRELESS_H__
#define __WIRELESS_H__

#include "config.h"

#define WL_HEAD                     0xFEu
#define WL_TAIL                     0xFFu
#define WL_TX_MAX                   64u

#define WL_CMD_POWER                0x01u
#define WL_CMD_TEMP_UP              0x03u
#define WL_CMD_TEMP_DOWN            0x04u
#define WL_CMD_SET_TEMP             0x05u
#define WL_CMD_JUMP_SET             0x06u
#define WL_CMD_QUERY_PROBE          0x07u
#define WL_CMD_UNIT                 0x09u
#define WL_CMD_QUERY_STATUS         0x0Bu
#define WL_CMD_QUERY_SET_ALL        0x0Du
#define WL_CMD_QUERY_ACT_ALL        0x0Eu
#define WL_CMD_BT_STATUS            0x24u
#define WL_CMD_FW_INFO              0x5Fu

#define WL_TEMP_MIN                 175u
#define WL_TEMP_MAX                 455u

void Wireless_Init(void);
void WirelessTask(void);

#endif
