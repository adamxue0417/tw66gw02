/**
  ******************************************************************************
  * @file    ota.h
  * @author 
  * @version V1.0
  * @date
  * @brief   OTA and IAP upgrade interface definitions.
  ******************************************************************************
  */
#ifndef __OTA_H__
#define __OTA_H__
#include <stdint.h>
#define UPGRADEaddr1      0x08005000		// offset:20KB
#define FLASH_BAK_ADDR		0x08024800
typedef struct 
{
  uint8_t McuVersionSucess;
	uint8_t WifiYmodeSucess;
//	u32 g_ota_recv_sum;
  uint16_t g_ota_pg_numid; 
	uint8_t g_ota_pg_nums;
	uint8_t  McuRestart;
	uint8_t  YModemFlag;
	uint32_t FileZise;    
	uint32_t PackageSize;
	uint8_t PackageDateReceiveFlash;
	uint8_t PackageDateReceiveReady;
	uint8_t PackagALLReceiveFlash;
	uint8_t SucessFlag;
	uint8_t iapMode;
	uint16_t crcSum;
	uint8_t OTAStartFlag;	
}_OTA_Date;
extern _OTA_Date OTA_Date;
void FLASH_ErasePage(uint32_t addr);
void SoftReset(void);
void download_success(void);
void iap_write_appbin(uint32_t appxaddr,uint8_t *appbuf,uint32_t appsize);
void iap_write_data_to_TouchScreen(uint16_t screenaddr, uint8_t *appbuf, uint8_t appsize);
void iap_write_32_data_to_TouchScreen(uint16_t fwid);
void iap_wirte_data_from_TouchScreen(uint16_t fwid);
void check_data_from_TouchScreen(void);
void ota_read_OK_from_TouchScreen(uint8_t* w_32_status);
void re_TouchScreen(void);
void stop_DGUS(void);
void update_date_to_TouchScreen(uint8_t updata_mode,uint8_t* snd_buf);
void ota_control(void);
#endif



